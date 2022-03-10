# CollectorSearcher
Searcher:
<br /> First thing this program needs to do is check if stdin is a pipe file.
Program takes one argument- number of uints to process (with optional unit). By process I mean load from stdin, check if this number showed up for the first time, if yes then send a binary structure (record with number and PID) to stdout. Program finishes working with status depends on repetition rate of processed numbers.
<br />
Collector:
<br /> This program is more complicated. It takes not one but six argumments which are responsible for:
<br /> 1. -d <path> path to file with data to process
<br /> 2. -s <wolumen> positive integer, with optional unit, how many numbers will be processed
<br /> 3. -w <blok> positive integer, with optional unit, how many numbers will be processed by child process
<br /> 4. -f <successes> path to file where achivements are written
<br /> 5. -l <raports> path to file where raports are written
<br /> 6. -p <prac> positive integer, max number of child processes
<br /> Program makes <prac> child processes and passes to each <blok> as argument. As long as child proces has died with status [0, 10] and successes file is filled in less than 75% program needs to replace each dead child with new one. Moreover, Collector needs to report creation of each of them and dead of ones which finished with status [0, 10]. Program uses two pipes to communicate with its children. 
<br />Collector copies from file specified by <path> to out pipe 2*<wolumen> bytes of data. It also needs to read result which are sent from its children - searchers and check if any of them has died. All operations are in non-blocking mode. Whern Collector reads results from children it also saves a PID in index of number in succes file. 
