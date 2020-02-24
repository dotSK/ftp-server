project('ftp-server', 'c')

inc = include_directories('include')
sources = [
    'src/main.c',
    'src/data_worker.c',
    'src/ftp_functions.c',
    'src/ftp_utils.c',
    'src/str_buf.c'
]

executable(
    'ftp',
    sources,
    include_directories: inc
)