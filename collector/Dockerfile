FROM python:3.6.10

RUN python -m pip install pyserial influxdb

COPY collector.py /collector.py

ENTRYPOINT ["python", "/collector.py"]
