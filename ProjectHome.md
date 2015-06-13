# Updates #
  * **2010-11-11** New s26 book. This one has **80001** draws proven to 29 empty! Finding new draws is now beginning to get harder. But there's still a great deal of nodes at 30 empty with score -2. These could turn out to be draws too. Anybody with a large number of cpu's who want to take over should contact me (dlidstrom at gmail dot com).
  * **2010-11-04** Uploaded new binary. Improvements for analysing books with smaller depth. Also, now prioritizes lines with shorter depth when total scores are equal. This will hopefully find new draws faster. Will soon upload a new s26 book with 80000 draws!

---



# Introduction #
This is the Ntest engine, developed originally by Chris Welty. The engine is capable of playing online at the Generic Game Server (GGS) located at the University of Alberta, Canada. See http://www.cs.ualberta.ca/~mburo/ for more details about that.

# Distributed Opening Book Building #
I have created a simple client/server system to enable distributed opening book building. The server generates a list of candidate lines by using a method called drop-out expansion. This method prioritizes long lines with scores close to 0, to find many draws, but also shorter lines that are further from 0.  This enables the book to expand promising lines as well.

# Example #
## Server ##
First you need to start the server. Choose a level (depth) and a port:
```
C:\ntest-server> type parameters.txt
32 3.2
JA s26 0 0 0 0 1 1 -1 -1
JA s20 0 0 0 0 1 1 -1 -1
# coeff params black white black_rand white_rand show_book_black sbw ....
# negamax showbook_info_me showbook_info_opponent
C:\ntest-server> ntest.exe b11 s26 300 1 server 1234
```
You need to edit the second line of parameters.txt with the level you intend to use. The first line specifies the size of the hashtable and the approximate processor speed.
  * b11 is the command for book expansion
  * s26 is the level (depth 26)
  * 300 is to only generate lines that have a score within +3/-3 discs. It speeds up the generation of new lines when the book grows big and there are thousands of lines.
  * 1 is just a placeholder, not used.
  * 1234 is the server port.
## Client(s) ##
Clients are also launched using ntest.exe. You can launch one or more clients but each needs to be in its own directory. They can even be on separate machines - the whole point of distributed computing ;-). Here's how you would connect to the server:
```
C:\ntest-client-1> type parameters.txt
128 3.2
JA s26 0 0 0 0 1 1 -1 -1
JA s20 0 0 0 0 1 1 -1 -1
# coeff params black white black_rand white_rand show_book_black sbw ....
# negamax showbook_info_me showbook_info_opponent
C:\ntest-client-1> ntest.exe b11 s26 300 1 client localhost 1234
```
The parameters are similar to the server parameters.

This should kick off the server which starts by generating lines. If the book is empty the first move to expand will be D3. After a while though the client will synchronize and the server will have more lines to prioritize next. Check variations.txt in the server directory to see the working queue.

## Status ##
The server writes a few files to enable simple monitoring.
  * varations.txt contains the lines queued for expansion
  * stats/ will contain files named after each client, that contain the number of nodes synchronized by each client

## Tips ##
You should download one of the big books in the downloads section. Place the book file in your server's Coefficients directory. This will save you a couple of years of computing time ;-).

# Compilation Instructions #
  * Install a recent version of Boost. I have used 1.41 successfully.
  * Open othello.vsprops and set the paths to Boost. Mine looks like this:
```
	<UserMacro
		Name="BOOST_INCLUDE_DIR"
		Value="C:\Users\Daniel.SBG\Programming\boost_1_41_0"
	/>
	<UserMacro
		Name="BOOST_LIB_DIR"
		Value="C:\Users\Daniel.SBG\Programming\boost_1_41_0\stage\lib"
	/>
```
  * Open othello.sln using Visual Studio 2008 (Professional or Express should be fine)
  * Press the Build button.
# NBoard/NBrowser #
These applications can be used to browse the book. NBoard connects to a version of ntest that can even analyze positions too!

# Help #
It would be nice to have the ntest books in WZebra format as well. I know there are converters out there so if someone would like to help, I would be most grateful!

# Troubleshooting #
## This application has failed to start because MSVCP100.dll was not found ##
If this happens it means you need to install the [redistributable](http://www.microsoft.com/downloads/details.aspx?familyid=A7B7A05E-6DE6-4D3A-A423-37BF0912DB84) package from Microsoft.