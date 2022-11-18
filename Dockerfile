FROM highperformancecoder/builttravisciimage
COPY . /root
RUN cd /root && make -j4 AEGIS=1

