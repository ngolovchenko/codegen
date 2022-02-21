Code Generator Project
----------------------

ABOUT
-----
This repository contains assembly code generators for the 
Microchip PIC/Scenix SX microntrollers. The project originated
by James Newton (http://www.massmind.org/) and Nikolai Golovchenko
(https://www.golovchenko.org) sometime in the year 2000. Many ideas
came from discussions on the PICList Mailing list. The code
was published on github in 2021.


BUILDS
------
The code generators were originally designed only to run on a web server
as cgi-bin applications. There are more ways to run the code generators now:

1. Command-line application.
2. cgi-bin application for server operation.
3. WebAssembly inside the browser.

Supported build platforms include Windows and Linux. Any client OS can be 
used for running the web-based applications (cgi-bin or WebAssembly).


COMMAND-LINE APPLICATION
------------------------
* Windows
Open the Visual Studio 2022 solution file codegen.sln. Build all or some of
the included projects.

* Linux
To build all code generators: 
  make clean      # optional
  make all

To build a single code generator:
  cd constdivmul  # (other options: delay, keyword)
  make clean      # optional
  make all

The output binaries are placed in:
  constdivmul/constdivmul
  delay/delay
  keyword/keyword


CGI-BIN APPLICATION
-------------------
Use the command-line binaries by following the steps above. They
automatically enter the cgi-bin mode by checking the environment
variables.

The exact web server setup may vary on different systems. Typically,
the binary has to be copied to the /cgi-bin directory. The binaries
must have execution permissions (chmod 755).

As an example, a local web server can be used for testing:

* Start the web server (and return to console)
  cd public_html
  python -m http.server 8080 --bind 127.0.0.1 --cgi &

* Copy executables
  mkdir cgi-bin
  cp ../builds/constdivmul cgi-bin/
  cp ../builds/delay cgi-bin/
  cp ../builds/keyword cgi-bin/
  chmod 755 cgi-bin/*

* Open the starting page in the browser
  http://127.0.0.1:8080/index.html

* Terminate the server process when done.

Note: when using Windows, the .exe extension needs to be handled.
For example, the extension can be removed.


WEBASSEMBLY APPLICATION
-----------------------
The application is embedded in an html file and can be opened in
a supporting browser. As of 2022, that includes Brave, Chrome, 
Firefox, Safari, and others.

Build instructions for Linux (tested in Ubuntu WSL2 on Windows 11):
* Install WebAssembly compiler: 
  https://emscripten.org/docs/getting_started/downloads.html
  cd ~
  source ./emsdk/emsdk_env.sh

* Build (all code generators)
  make clean
  make wasm

* Open the following html pages in your browser and execute:
  constdivmul/constdivmul.html
  delay/delay.html
  keyword/keyword.html






