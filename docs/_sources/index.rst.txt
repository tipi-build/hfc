.. title:: HermeticFetchContent Reference Documentation


Introduction
############

Hermetic FetchContent is an extension of CMake's FetchContent module, designed for improved 
management and isolation of project dependencies. It enables dependencies to be built in hermeticity,
supports a variety of build systems for dependencies (including CMake, GNU Autotools, and OpenSSL).

Hermetic FetchContent ensures cleaner, more predictable and easier to maintain builds in complex 
CMake projects.


Modules
======= 

.. toctree::
  :maxdepth: 1
  
  /modules/HermeticFetchContent

Recipes
======= 

.. toctree::
  :maxdepth: 2
  
  /recipes/HowToAddALibrary
  /recipes/HowToDefineBuildEnvironments
  /recipes/ControllingBuildParallelism


Index and Search
################

* :ref:`genindex`
* :ref:`search`
