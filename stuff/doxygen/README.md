# Doxygen documentation for TNeo

Having Doxygen and latex installed, here's how to generate documentation for a
new release:

```
$ make
$ bash ./publish_doc_release.sh v1.09
```

(where `v1.09` is the git tag for the new version)

To generate docs for a development (not yet released) version, use `bash
./publish_doc_dev.sh` instead of the last command.

TODO: make a Dockerfile with all the dependencies.
