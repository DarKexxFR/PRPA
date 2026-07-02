##

```
export GST_PLUGIN_PATH=$(pwd)/build  # Chemin vers le plugin
```


# Install (tested with gcc-11 and gcc-10)

1. Untar the archive

```
tar xf prpa-src.tar.gz

```

2. Create your build directory

2-b. Install project dependencies with a nix shell


2-a. Install project dependencies with the [conan](https://docs.conan.io/en/latest/introduction.html) package manager.

```
conan profile update settings.compiler.libcxx=libstdc++11 default
conan install .. --build missing
```





4. Build the project (in Debug or Release) with cmake

```
cmake -S . -B /tmp/build -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/build
```


