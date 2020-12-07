so修复相关
如果是从内存中dump出来的，需要加上 -m dumpBase. 可以自动完成重定位的修复。
./SoFixer -s /Users/ring/Desktop/xxx/lib/armeabi-v7a/libxxx-6.4.201.so  -a 32 -o /Users/ring/Desktop/xxx/lib/armeabi-v7a/libxxx-6.4.201.so
添加arm64架构的支持：./SoFixer -s /Users/ring/Desktop/xxx/lib/arm64-v8a/libxxx-6.4.201.so  -a 64 -o /Users/ring/Desktop/xxx/lib/arm64-v8a/libxxx-6.4.201.so

原理参考下面的文章
TK so修复参考[http://bbs.pediy.com/thread-191649.htm]
