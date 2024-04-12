"""
Setup file for the Xinshuo's Python Toolbox package.
"""

from setuptools import find_packages, setup
setup(
    name="xinshuo-py-toolbox",
    version="1.0.0",
    author="Xinshuo Weng",
    author_email="xinshuo.weng@gmail.com",
    description="A Python toolbox that contains common help functions for stream I/O, image & video processing, and visualization. All my projects depend on this toolbox.",
    long_description=open("readme.md", "r", encoding="utf-8").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/xinshuoweng/Xinshuo_PyToolbox",
    packages=find_packages(),
    install_requires=[
        'conversions==0.0.2',
        'glob2==0.7',
        'google-api-python-client==2.125.0',
        'httplib2==0.22.0',
        'matplotlib==3.8.2',
        'numba==0.59.0',
        'numpy==1.24.3',
        'oauth2client==4.1.3',
        'opencv-python==4.9.0.80',
        'opencv-python-headless==4.9.0.80',
        'Pillow==10.3.0',
        'pytest==7.4.4',
        'scikit-image==0.23.1',
        'scikit-video==1.1.11',
        'scipy==1.13.0',
        'setuptools==65.5.0',
        'terminaltables==3.1.10'
    ],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)

