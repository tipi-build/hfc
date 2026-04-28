{
  "variables": { },
  "builders": [
    {
      "type": "docker",
      "image": "tipibuild/tipi-ubuntu:{{cmake_re_source_hash}}",
      "commit": true
    }
  ]
}