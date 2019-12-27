from distutils.core import setup, Extension

audiosync = Extension(
    'audiosync',
    define_macros = [('DEBUG', '1')],
    include_dirs = ['/usr/local/include'],
    libraries = ['m', 'pthread', 'fftw3'],
    library_dirs = ['/usr/local/lib'],
    sources = ['src/bind.c', 'src/audiosync.c', 'src/cross_correlation.c',
               'src/download/linux_download.c', 'src/capture/linux_capture.c'])

setup (name = 'audiosync',
       version = '1.0',
       ext_modules = [audiosync])
