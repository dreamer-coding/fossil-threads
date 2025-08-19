dir = include_directories('.')
cc = meson.get_compiler('c')

fossil_threads_lib = static_library('fossil-thread',
    files('thread.c', 'mutex.c', 'cond.c', 'ghost.c'),
    install: true,
    dependencies: [cc.find_library('m', required: false), dependency('thread')],
    include_directories: dir)

fossil_threads_dep = declare_dependency(
    link_with: [fossil_threads_lib],
    include_directories: dir)
