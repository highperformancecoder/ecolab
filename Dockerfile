FROM highperformancecoder/builttravisciimage
COPY . /root
RUN cd /root && xvfb-run make DEBUG=1 AEGIS=1

