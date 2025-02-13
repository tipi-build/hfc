.. title:: HermeticFetchContent / Recipes / Controlling Build Parallelism

Controlling Build Parallelism
#############################

Because of the complex nature of the build graph in Hermetic FetchContent you may want to adjust the build
parallelity in some situations like CI or shared build nodes.

Hermetic FetchContent does use a combination of the following parameters to set the build parallelism:

 - the number of CPU cores available on your system
 - the value of the environment variable ``CMAKE_BUILD_PARALLEL_LEVEL``
 - the value supplied to ``cmake --build`` via the ``-j <jobs>`` / ``--parallel <jobs>`` argument

Generally note that dependencies that are made available at **build time** will have their own, separate
allocation of CPU ressources as these dependent builds are generally unaware of the ressource allocations
of peer and parent projects. This means that you may end up with sever over-loading of the system depending
on the build graph of your project and available ressources.

When no setting is provided, Hermetic FetchContent will defer to the invoked build system (``ninja`` or ``make``
for example) to select the build parallelism (which typically fall back to the number of CPU cores).

The value provided via ``CMAKE_BUILD_PARALLEL_LEVEL`` is taken into account by CMake throughout the build graph,
meaning that the set value is used for each of the builds that make up the graph of your project.

The value passed to CMake via the ``-j <jobs>`` / ``--parallel <jobs>`` argument are taken into account **only
for the top level build graph**. This detail enables you to control the build parallelism for the project
and the dependency builds independently:

.. code-block:: bash

  # $PWD = project root
  export CMAKE_BUILD_PARALLEL_LEVEL=16
  cmake -S . -B build/release/ -DCMAKE_BUILD_TYPE=Release ...
  export CMAKE_BUILD_PARALLEL_LEVEL=2
  cmake --build . -j10

The example above will have the following behavior

 - all dependencies **made available at configure time** will use 16 CPU cores (which is not an issue as they are run in sequence)
 - during the build
   - dependencies **made available at build time** will use 2 CPU cores
   - the top level build graph (your project) will use 10 CPU cores

