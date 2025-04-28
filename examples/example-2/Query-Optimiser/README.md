# <div align="center">Query Optimiser</div>

> Repository for End term submission for Database Management Systems Lab course (CS39202) offered in Spring semester 2023, Department of CSE, IIT Kharagpur.

<!-- PROJECT LOGO -->
<br />
<div align="center">
    <!-- <img width="200" src="https://user-images.githubusercontent.com/86282911/230894496-b9402384-bf0a-4bf7-afbf-2207aa2d31be.png">
   -->
  <p align="center">
    <i>Compiling queries</i>
    <br />
    <br />
    <img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=C%2B%2B&logoColor=white" />
    <img src="https://img.shields.io/badge/Python-FFD43B?style=for-the-badge&logo=python&logoColor=blue"/>
    <img src="https://img.shields.io/badge/Streamlit-FF4B4B?style=for-the-badge&logo=Streamlit&logoColor=white">
    <br />
    <br />
    <a href="https://github.com/outer-rim/Query-Optimiser/issues">Report Bug</a>
    ·
    <a href="https://github.com/outer-rim/Query-Optimiser/issues">Request Feature</a>
  </p>
</div>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#directory-structure">Directory structure</a></li>
        <!-- <ul>
          <li><a href="#chromium-based-browsers">Chromium Based Browsers</a></li>
          <li><a href="#firefox">Firefox</a></li>
        </ul>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
    <li><a href="#miscelleneous">Miscelleneous</a></li>     -->
  </ol>
</details>

## About The Project

This project is a compiler.

Team members:

- Archit Mangrulkar - 20CS10086
- Ashwani Kumar Kamal - 20CS10011
- Hardik Pravin Soni - 20CS30023
- Shiladitya De - 20CS30061
- Sourabh Soumyakanta Das - 20CS30051

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

A quick introduction of the minimal setup you need to get the application up

Install the gcc package along with flex and bison (following commands are applicable for Ubuntu based systems)

```shell
sudo apt install build-essentials
sudo apt install flex bison
make run
pip install -r requirements.txt
streamlit run deploy.py
```

### Directory Structure

```shell
.
├── call.py
├── deploy.py
├── inp.txt
├── Makefile
├── README.md
├── requirements.txt
├── test.l
├── test_target_translator.cxx
├── test_translator.cxx
├── test_translator.h
├── test.y
└── tmp
    ├── in.txt
    └── out.txt

```
