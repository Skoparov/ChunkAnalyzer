# ChunkAnalyzer

Socket based file chunks diversity calculator. Essentially just splits files into chunks, concurrently sends them thought a socket and collects unique chunks on the other end, calculating their number.

Consists of two projects: Analyzer(server) and Loader (client)

Analyzer params:
- Output file name (arbiatry, will use stdout if not defined)
- Port (arbitary, will use 7777 is not deined)

Loader params:
- Server address
- Port
- Dirs one by one
