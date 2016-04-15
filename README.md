# Osimis Web Viewer plugin

This project is a fork from [Orthanc WebViewer plugin](http://www.orthanc-server.com/static.php?page=web-viewer).  It adds measure tools, multiple series view and split-pane.  You may try it [here](http://osimisviewer.osimis.io)

This repo also contains a [Docker](https://www.docker.com/) container definition for Orthanc and all its main FOSS plugins including the Osimis WebViewer that you can find [here](https://hub.docker.com/r/osimis/orthanc-webviewer-plugin/builds/)

## Usage

To use the docker container, follow [this procedure](https://bitbucket.org/snippets/osimis/eynLn).

## Build & development

This repo contains the plugin C++ code.  The html/js code lies in [this repo](https://bitbucket.org/osimis/osimis-webviewer).  A jenkins job publishes the html/js generated by gulp into this repository (in subtrees/WebViewer.git).

### Pulling changes from Fork origin
To retrieve changes from original mercurial repo to git fork, uses
- https://github.com/fingolfin/git-remote-hg 
-> This is insanely awesome, thanks god !



### Merging dev into master:

until we change the dev workflow :-)

- after a push in JS[dev], jenkins triggers a gulp build.
- merge JS[dev] in JS[master] (but it's actually not very useful since JS[master] is not really used)
- wait jenkins complete the generation of the JS[build-dev] branch ("compiled" JS) 
- wait jenkins updates C++[dev]/subtrees/osimis-webviewer with JS[build-dev]
- merge C++[dev] into C++[master] (keep in mind that the C++[master]/subtrees is actually pointing to JS[build-dev]