### Documentation

To generate the documentation you need Sphinx installed in your Python packages:

```bash
pip install -U sphinx
```

To generate the documentation:

```bash
mkdir -p build_docs
cd build_docs/
cmake .. -GNinja
ninja
```

To update the in-project documentation:

```bash
rm -r ../../public/documentation/*
cmake --install . --prefix ../../public/documentation/
```
