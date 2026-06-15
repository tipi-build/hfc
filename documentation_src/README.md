### Documentation

To generate the documentation you need Sphinx installed in your Python packages:

```bash
pip install -U sphinx
```

> ### All in one helper script:
> 
> ```bash
> ./update_documentation.sh
> ```
> Performs all the required steps

#### Alternative: using uv (no virtualenv needed)

Using [uv](https://docs.astral.sh/uv/), you can generate the documentation without installing Sphinx or managing a virtualenv.

> ### All in one helper script:
>
> ```bash
> uv run --with sphinx ./update_documentation.sh
> ```

### Step by step:

To generate the documentation:

```bash
mkdir -p build_docs
cd build_docs/
cmake .. -GNinja
ninja
```

To update the in-project documentation:

```bash
rm -r ../../docs
mkdir -p ../../docs
cmake --install . --prefix ../../docs/
touch ../../docs/.nojekyll # required for gh-pages
```
