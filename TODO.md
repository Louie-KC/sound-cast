# Sound Cast TODO

# Core
- [ ] Define Core API
    - [ ] Context struct
    - [ ] Initialisation, shutdown
- [ ] Platform layer
    - [ ] Windows (Win32 https://learn.microsoft.com/en-us/windows/win32/directshow/selecting-a-capture-device)
    - [ ] MacOS (AvFoundation https://developer.apple.com/documentation/avfoundation/audio-and-video-capture)
- [ ] Logger
    - [x] Define `LOG_...` macros for fatal, error, warn, info and debug logging.
    - [ ] Allow for varargs to be passed in and formatted into log.
- [ ] Assertions
    - [x] Create assertion macro.
        - [x] Optional message variation.
        - [ ] Non-fatal level assertions.
# CLI
- [ ] Argument definitions and parsing
