cd librpc
nmake %1
cd ..\rpcgen
nmake %1
cd ..\service
nmake %1
cd ..\rpcinfo
nmake %1
cd ..\test
nmake %1
cd ..
