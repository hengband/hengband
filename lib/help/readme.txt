=== Notes on Zangband Documentation ===

This file sets out basic information regarding the official Zangband
documentation package produced by the Zangband DevTeam. 


--- Documentation Packages ---

There are currently two documentation packages planned. The basic
package will include non-spoiler information necessary for a new player
to learn all he needs to know about playing Zangband and will be
included in all official Zangband source and binary releases. The
extended package will included additional spoiler files and will be
available for download separately. Care will be taken to ensure that
the two packages merge together properly.


--- Available Formats ---

The Zangband documentation packages will be made available in three
formats for download, plain text, html and .chm (for windows users).
The plain text version will run from the in-game '?' command. The html
version will be placed on the web and will be made available for
download to allow local browsing. The *.chm file is a windows specific
format which allows adaption of html documentation packages to be used
in windows helpfiles. It introduces full text searching and many other
nice features.


--- Reporting Errors / Suggestions for Improvement ---

While every effort will be taken to ensure that the documentation
packages are accurate and up to date, it is inevitable that some
errors will be made. If you find an error or have a suggestion to
improve documentation, please post your report or idea to the Zangband
Development list by mailing to zangband@onelist.com. Alternatively, you
may post your report or idea to rec.games.roguelike.angband.


--- Rules for Creating Documents ---

Certain general rules have been applied in creating the official
documentation for Zangband. The DevTeam welcomes contributions by
players and if you wish to contribute a spoiler or other helpfile we
would appreciate your adherence to the following guidelines. We reserve
the right to modify your file but you will retain credit as the
original author.

1.  All documents should be named using the standard 8.3 format since
    Zangband is played on systems which do not support long filenames.
2.  Menu files should be named *.hlp, non-spoiler help files *.txt and
    spoiler helpfiles *.spo.
3.  All documents should be in a fixed font, plain text format.
4.  Line length should be between 70 and 75 characters.
5.  Please use US spelling unless it directly conflicts with the game
    (for example the 'Spectre' race).
6.  No indenting of the first line of paragraphs.
7.  Left justification.
8.  Document titles are denoted by '=== Document Title ==='.
9.  Section headings are denoted by '=== Section Heading ==='.
10. Sub-section headings are denoted by '--- Sub-section Heading ---'.
11. Information on document authors and editors and revisions should
    be included at the end of each document.
12. Indenting increments by 4 or 5 spaces each time.


=== Hyperlinks ===

Zangband supports links between help files as follows. At the point in
the text where you wish to insert a link, place the text '[#]' where '#'
is any letter or digit ('a' and 'A' are distinct for this purpose).
This is to tell the reader which key to press to activate the link.
Then, at the end of the file, place the following text (including the
asterisks) where 'target_filename' is the name of the file you wish to
link to:

     ***** [#] target_filename

Now, when the user presses '#', the helpfile system will open up the
target file. Opening the new file will flush the previous links so
'#' may be reused.

Note: because various operating systems use different conventions to
denote directory structures, Zangband does not support target file
names like './spoiler/races/races.spo'. This means that all help files
should be kept in the lib/help or lib/info directory.

We are currently testing methods of linking to a particular section
in a document and may also implement histories to allow 'back' and
'forward' actions similar to web browsers.

--- Tags ---

A "tag" is something like a bookmark, it marks a line for reference by
a hyperlink.  A tag is specified with:

	***** <tag_name>

(the name of the tag can not be longer than 15 characters).

You can then jump to the line marked with the tag with:

	***** [1] target_filename#tag_name

This command jumps to the line with the tag "tag_name" in the file
target_file.  You can jump inside the current file too (just use the
filename of the current file in the link).

-- 
Original    : Zangband DevTeam
Last update : January 13, 2000

