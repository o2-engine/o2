## Logging
Log output uses a hierarchy of log channels `o2::LogStream`. Each channel has a string identifier and a list of child channels.

Output to a child channel also outputs to its parent.

The top-level channels are console output and file output.

You can create your own channels and bind them to others.

Log messages are divided into 3 severity types:
- regular message
- warning
- error
