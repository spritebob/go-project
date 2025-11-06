Setup with vcpkg:
===
If you don't already have vcpkg, clone the repository from here: https://github.com/microsoft/vcpkg

If you have already downloaded vcpkg then go to the local vcpkg repository and:
```
git pull
./bootstrap-vcpkg.sh
./vcpkg.exe upgrade --no-dry-run
```

Now install the following packages:
```
./vcpkg.exe install glew
./vcpkg.exe install freeglut
./vcpkg.exe integrate install
```

The project is now ready to compile and run in Visual Studio 2019!
