## Logging
Log output uses a hierarchy of log channels `o2::LogStream`. Each channel has a string identifier and a list of child channels.

Output to a child channel also outputs to its parent.

The top-level channels are console output and file output. The file is `log.txt` in the working directory, opened on the first message; `o2Debug.SetLogFile(name)` redirects it, which tools started from an application's folder do before their first message (the assets builder writes `AssetsBuilder.log`) so they never truncate the application log.

You can create your own channels and bind them to others.

Log messages are divided into 3 severity types:
- regular message
- warning
- error
