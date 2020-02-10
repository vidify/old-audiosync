from setuptools import setup, Extension


defines = []
args = ['-fno-finite-math-only']

# "Debug mode" flags by uncommenting them
# defines.append(('DEBUG', '1'))

audiosync = Extension(
    'vidify_audiosync',
    define_macros = defines,
    extra_compile_args = args,
    include_dirs = ['include'],
    libraries = ['m', 'pthread', 'fftw3'],
    library_dirs = ['/usr/local/lib'],
    sources = ['src/bind.c', 'src/audiosync.c', 'src/cross_correlation.c',
               'src/ffmpeg_pipe.c', 'src/download/linux_download.c',
               'src/capture/linux_capture.c']
)

setup(
    name='vidify-audiosync',
    version='0.1.4',
    description='Vidify extension to synchronize a YouTube video with the'
    ' audio playing on your device.',
    long_description=open('README.md', 'r').read(),
    long_description_content_type='text/markdown',
    url='https://github.com/marioortizmanero/vidify-audiosync',
    license='MIT',

    author='Mario O.M.',
    author_email='marioortizmanero@gmail.com',
    maintainer='Mario O.M.',
    maintainer_email='marioortizmanero@gmail.com',

    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: End Users/Desktop',
        'Topic :: Multimedia :: Sound/Audio :: Analysis',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C'
    ],
    keywords='audio synchronization cross-correlation pearson coefficient fft'
    ' audio-processing ffmpeg youtube-dl fftw',
    python_requires='>=3.6',
    ext_modules = [audiosync]
)
