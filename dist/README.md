Pushing a `release/vX.Y.Z` tag makes the `.github/workflows/python-wheels.yml`
workflow build the wheels and publish them to PyPI. It does not publish the
source distribution: build it with `python -m build --sdist` from the main
directory and upload it with `twine upload dist/*.tar.gz`.
