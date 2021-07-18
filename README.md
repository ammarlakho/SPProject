# sp-project

<!-- Output copied to clipboard! -->

<!-----
NEW: Check the "Suppress top comment" option to remove this info from the output.

Conversion time: 0.688 seconds.


Using this Markdown file:

1. Paste this output into your source file.
2. See the notes and action items below regarding this conversion run.
3. Check the rendered output (headings, lists, code blocks, tables) for proper
   formatting and use a linkchecker before you publish this page.

Conversion notes:

* Docs to Markdown version 1.0β30
* Sun Jul 18 2021 04:22:28 GMT-0700 (PDT)
* Source doc: SPReport
* Tables are currently converted to HTML tables.
----->



# Client Server Application

<p style="text-align: right">
<strong>Ammar Lakho 18055</strong></p>



## Help


### **Client** 


<table>
  <tr>
   <td><strong>Input </strong>
   </td>
   <td><strong>Result</strong>
   </td>
  </tr>
  <tr>
   <td>add/sub/mul/div &lt;list>
   </td>
   <td>Prints the answer on the screen
   </td>
  </tr>
  <tr>
   <td>run &lt;process>
   </td>
   <td>Creates a new process and adds it to activeList and allList.
   </td>
  </tr>
  <tr>
   <td>kill &lt;pname>
   </td>
   <td>Terminates the first instance of the process “pname”.
   </td>
  </tr>
  <tr>
   <td>kill &lt;pid>
   </td>
   <td>Terminates the process with process ID=pid
   </td>
  </tr>
  <tr>
   <td>listActive
   </td>
   <td>Prints pid, name and start_time for each active process executed by the client.
   </td>
  </tr>
  <tr>
   <td>listAll
   </td>
   <td>Prints pid, name, start_time, end_time, and duration(in seconds) for each process executed by the client.
   </td>
  </tr>
</table>



### **Server** 


<table>
  <tr>
   <td><strong>Input </strong>
   </td>
   <td><strong>Result</strong>
   </td>
  </tr>
  <tr>
   <td>listConn
   </td>
   <td>Prints socketfd, IP and port# for each client.
   </td>
  </tr>
  <tr>
   <td>print &lt;msg>
   </td>
   <td>Prints &lt;msg> on each client’s terminal.
   </td>
  </tr>
  <tr>
   <td>print &lt;msg> &lt;fd>
   </td>
   <td>Prints &lt;msg> on the terminal of the client with socketfd=fd.
   </td>
  </tr>
  <tr>
   <td>listProcess
   </td>
   <td>Prints the activeList for each client
   </td>
  </tr>
  <tr>
   <td>listProcess &lt;fd>
   </td>
   <td>Prints the activeList for client with socketfd=fd.
   </td>
  </tr>
</table>



## How to Run


### Client

_./client IP Port#_


### IP address of the computer running the server should be provided alongside the port that the server has made available for communication.


### Server

_./server Port#_


## Architecture

### Client

The client process connects to the server using an IP address and a port number provided as arguments.

After a successful connection, the client process breaks into 2 separate threads:



1. **Command Thread**: This thread reads a command from STDIN and writes it to the socket.
2. **Result Thread**: This thread reads the response from the server on the socket and writes it to STDOUT.


### Server

The server process has 2 threads:



1. **Command Thread**: This thread reads a command from STDIN, understands the command, and writes a response to STDOUT.
2. **Accept Thread**: This thread accepts a connection from a client and if that is successful, it fork()s and the child process that is spawned becomes the client handler for the client that has just been accept()ed.

**Client Handler**: The client handler process reads from the socket to get the command entered by the client, understands it, and writes an appropriate response to the socket.


## Achievements



1. Handled most errors and made the client aware of the error. 
2. Handled unexpected termination of client process and client handler process.
3. If the main server process(conn) crashes, the connected clients still remain connected to the server(client handler) and can execute their commands. Only new connections won’t be handled.
4. Ensured no zombie processes exist to minimize wastage of resources. 
