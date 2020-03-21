import os
import re
import sys
import json
import time
import serial
import logging
import datetime
import requests.exceptions
from influxdb import InfluxDBClient

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

logging.basicConfig(level=logging.DEBUG)

CONFIG_FILE = "logger.json"
PATTERN = re.compile(b"(?P<name>\w+): (?P<temperature>[0-9]+\.[0-9]+)C")
READ_SIZE = 10
RETRY_TIMEOUT = 5

class FanControllerCollector:

    def __init__(self, client, tty, sensor_names):
        self._client = client
        self._tty = tty
        self._sensor_names = sensor_names
        self._serial = None
        self._buffer = b""

    def _handle_match(self, match):
        dict = match.groupdict()
        name = dict["name"].decode()
        temperature = float(dict["temperature"])

        sensor_name = self._sensor_names.get(name, name)
        timestamp = datetime.datetime.now().isoformat()

        json_body = [
            {
                "measurement": "fan-controller",
                "tags": {
                    "name": sensor_name,
                },
                "time": timestamp,
                "fields": {
                    "temperature": temperature
                }
            }
        ]

        try:
            self._client.write_points(json_body)
        except requests.exceptions.ConnectionError:
            logger.exception("writing measurement failed")

    def _open(self):
        while True:
            try:
                return serial.Serial(self._tty, 9600)
            except serial.serialutil.SerialException:
                logger.warning("failed to open %s, trying again in %ss" % (self._tty, RETRY_TIMEOUT))
                time.sleep(RETRY_TIMEOUT)

    def _read(self):
        if self._serial is None:
            self._serial = self._open()

        try:
            return self._serial.read(READ_SIZE)
        except serial.serialutil.SerialException:
            logger.warning("failed to read %s, reopening" % self._tty)
            self._serial.close()
            self._serial = None
            return b""

    def _read_line(self):

        while True:
            self._buffer += self._read()
            yield from self._read_buffer()

    def _read_buffer(self):
        lines = []

        # read lines from buffer until \n is not found
        while True:
            try:
                index = self._buffer.index(b"\n") + 1
                lines.append(self._buffer[:index].strip())
                self._buffer = self._buffer[index:]
            except ValueError as e:
                return lines

    def run(self):
        self._buffer = b""

        try:
            for line in self._read_line():
                match = PATTERN.match(line)
                if match:
                    self._handle_match(match)
        finally:
            if self._serial is not None:
                self._serial.close()

def read_config():
    config = {}

    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as h:
          config = json.load(h)

    if "tty" not in config:
        sys.exit("tty not defined in %s" % CONFIG_FILE)

    tty = config["tty"]
    config.setdefault("sensors", [])

    sensor_names = {
        sensor["id"]: sensor["name"]
        for sensor in config["sensors"]
    }

    return tty, sensor_names


if __name__ == "__main__":
    tty, sensor_names = read_config()
    logger.info("config:\n  tty: %r\n  sensors: %r" % (tty, sensor_names))

    logger.info("connecting to influxdb")
    client = InfluxDBClient("localhost", 8086, "user", "password", "db")

    logger.info("listening")

    collector = FanControllerCollector(client, tty, sensor_names)
    collector.run()
