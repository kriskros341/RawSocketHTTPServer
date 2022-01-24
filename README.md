# Simple HTTP server and demo

This is my final project for first semester. School documentation is in "dokumentacja projektu zaliczeniowego.docx"

## Description

The library gives access to a Server class that takes care of all the communication asynchronously. It doesn't require user to know anything about networking, while also providing near total control. 

Its simplistic, unopinionated API is inspired by tools I use, like Tornado (class handlers), NodeJS HTTP module and FastAPI (very simple function handlers, middleware model).

Provided demo serves as a template for implementing own endpoints. It uses PostgreSQL, but can be adjusted to any database, whether SQL or NoSQL. The server is totally db-agnostic.

## How it works

HTTP endpoints are defined as either functions that match the handlerFunction<> type or objects that inherit from HandlerClass<>. 

The server is initialized by providing context template and matching argument. The server may be initialized with custom port argument.

Every time you see <> the template that matches one defined on server should be applied. It's used for storing any type of global state you might want on the server that then gets passed to all the functions and methods as a last argument.

Endpoints are added to the server by .on() method. They are then saved in a Map that associates them with path and method.

The middleware is a set of decorators. Middleware obiects must extend MiddlewareFunctor<> obiect.

## How did I do this

I wanted to create a working server on lower level to better understand what's going on under the hood. I learned how to set up sockets in C from Beej's Guide to Network Programming, then created SocketServer class to take care of everything. 

It contains a virtual method called on every request for child class to deal with incoming Data. This way It's extensible for other protocols in the future.

The server itself consists of two files in includes directory, Server.h and Server.cpp. These two provide all the classes and functions needed. All the other files are either for testing or are a part of the demo todo app.

in current implementation every request creates a new thread. I learned about threads from Beej's guide to Interprocess Communication. It originally created subprocesses which, while simpler, can't share a global state and obiects as easily.

The project doesn't currently support encyption. An easy workaround is creating a reverse proxy that takes care of encryption.

Original plan also inluded a thread pool that would make request handling faster and safer, but I ran out of time and steam for IPC.

## How to install

To install the demo that also serves as a template you need to clone this repository.

To use the server you need to simply fetch Server.cpp and Server.h from the includes directory.

## How to run

The demo requires a PostgreSQL database with schema that can be infered from getTasks function working on local network.

When listening on port bellow 1000, running as administrator is required.

## License: 

lol. lmao.
