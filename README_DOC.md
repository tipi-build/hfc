### Documentation

To generate the documentation you need Sphinx installed in your Python packages:
```
pip install -U sphinx
```

To generate the documentation:

```
mkdir -p build_docs
cd build_docs/

cmake ../Utilities/Sphinx -GNinja -DSPHINX_HTML=ON -DSPHINX_QTHELP=$CMAKE_CI_SPHINX_QTHELP -DCMake_SPHINX_CMAKE_ORG=ON -DCMake_VERSION_NO_GIT=v0.0.0
ninja 
```


