/* R2.2 123.RI menu strings; cited offsets: spec/i123/menu_ri_offsets.tsv. */
#define I123_MENU_STR_BYTES 14062u
#define I123_MENU_LEVELS 82u
static const char menu_template_strings[] =
    "Worksheet\0"
    "Global  Insert  Delete  Column  Erase  Titles  Window  Status  Page  Learn\0"
    "Range\0"
    "Format  Label  Erase  Name  Justify  Prot  Unprot  Input  Value  Trans  Search\0"
    "Copy\0"
    "Copy a cell or range of cells\0"
    "Move\0"
    "Move a cell or range of cells\0"
    "File\0"
    "Retrieve  Save  Combine  Xtract  Erase  List  Import  Directory  Admin\0"
    "Print\0"
    "Print a range on a printer or to a print file\0"
    "Graph\0"
    "Type  X  A  B  C  D  E  F  Reset  View  Save  Options  Name  Group  Quit\0"
    "Data\0"
    "Fill  Table  Sort  Query  Distribution  Matrix  Regression  Parse\0"
    "System\0"
    "Leave 1-2-3 temporarily and use operating system\0"
    "Add-In\0"
    "Attach, Detach, Invoke or Clear 1-2-3 add-in programs\0"
    "Quit\0"
    "End the 1-2-3 session\0"
    "Global\0"
    "Format  Label-Prefix  Column-Width  Recalculation  Protection  Default  Zero\0"
    "Insert\0"
    "Insert blank column(s) or row(s)\0"
    "Delete\0"
    "Delete entire column(s) or row(s)\0"
    "Column\0"
    "Set-Width  Reset-Width  Hide  Display  Column-Range\0"
    "Erase\0"
    "Erase the entire worksheet from memory\0"
    "Titles\0"
    "Set horizontal or vertical titles\0"
    "Window\0"
    "Set split screen and synchronized scrolling\0"
    "Status\0"
    "Display worksheet settings\0"
    "Page\0"
    "Insert a row containing a page-break symbol above the cell pointer\0"
    "Learn\0"
    "Record keystrokes in the worksheet\0"
    "Format\0"
    "Fixed  Sci  Currency  ,  General  +/-  Percent  Date  Text  Hidden\0"
    "Label-Prefix\0"
    "Set global label alignment\0"
    "Column-Width\0"
    "Set global column width\0"
    "Recalculation\0"
    "Natural  Columnwise  Rowwise  Automatic  Manual  Iteration\0"
    "Protection\0"
    "Turn worksheet protection on or off\0"
    "Default\0"
    "Printer  Directory  Status  Update  Other  Autoexec  Quit\0"
    "Zero\0"
    "Change the way cells with a value of zero appear on the screen\0"
    "Fixed\0"
    "Fixed number of decimal places (x.xx)\0"
    "Sci\0"
    "Exponential format (x.xxE+xx)\0"
    "Currency\0"
    "Currency format ($x,xxx.xx)\0"
    ",\0"
    "Comma format (x,xxx.xx)\0"
    "General\0"
    "Standard format (x.xx or x.xxExx)\0"
    "+/-\0"
    "Horizontal bar graph format (+++ or ---)\0"
    "Percent\0"
    "Percent format (x.xx%)\0"
    "Date\0"
    "Date and time formats\0"
    "Text\0"
    "Display formula instead of value\0"
    "Hidden\0"
    "Do not display cell contents\0"
    "Left\0"
    "Left-align labels in cells\0"
    "Right\0"
    "Right-align labels in cells\0"
    "Center\0"
    "Center labels in cells\0"
    "Natural\0"
    "Recalculate in natural order\0"
    "Columnwise\0"
    "Recalculate column by column\0"
    "Rowwise\0"
    "Recalculate row by row\0"
    "Automatic\0"
    "Recalculate every time entries change\0"
    "Manual\0"
    "Press CALC (F9) to recalculate formulas\0"
    "Iteration\0"
    "Specify number of recalculation passes\0"
    "Enable\0"
    "Turn worksheet protection on\0"
    "Disable\0"
    "Turn worksheet protection off\0"
    "Insert one or more blank columns to the left of the cell pointer\0"
    "Row\0"
    "Insert one or more blank rows above the cell pointer\0"
    "Delete one or more columns\0"
    "Delete one or more rows\0"
    "Set-Width\0"
    "Specify width for current column\0"
    "Reset-Width\0"
    "Return current column to global column width\0"
    "Hide\0"
    "Hide a range of columns\0"
    "Display\0"
    "Display a range of hidden columns\0"
    "Column-Range\0"
    "Change the width of a range of columns\0"
    "Specify the width of a range of columns\0"
    "Return a range of columns to global column width\0"
    "No\0"
    "Do not erase the worksheet; return to READY mode\0"
    "Yes\0"
    "Both\0"
    "Freeze all rows and columns above and to the left of the cell pointer\0"
    "Horizontal\0"
    "Freeze all rows above the cell pointer\0"
    "Vertical\0"
    "Freeze all columns to the left of the cell pointer\0"
    "Clear\0"
    "Unfreeze all title columns and rows\0"
    "Split the screen horizontally at the current row\0"
    "Split the screen vertically at the current column\0"
    "Sync\0"
    "Synchronize scrolling in windows\0"
    "Unsync\0"
    "Scroll windows independently\0"
    "Return to full-screen display\0"
    "Specify a learn range in which to store keystrokes\0"
    "Cancel\0"
    "Cancel the currently specified learn range\0"
    "Erase the contents of the learn range\0"
    "Printer\0"
    "Specify printer interface and default settings\0"
    "Directory\0"
    "Specify default directory\0"
    "Display all default settings\0"
    "Update\0"
    "Save new default settings in configuration file\0"
    "Other\0"
    "International  Help  Clock  Undo  Beep  Add-In\0"
    "Autoexec\0"
    "Run autoexecute macros when files are retrieved\0"
    "Return to READY mode\0"
    "Interface\0"
    "Specify printer interface\0"
    "AutoLF\0"
    "Automatic linefeed at end of line\0"
    "Set default left margin\0"
    "Set default right margin\0"
    "Top\0"
    "Set default top margin\0"
    "Bot\0"
    "Set default bottom margin\0"
    "Pg-Length\0"
    "Set default number of lines per page\0"
    "Wait\0"
    "Wait for paper change at end of each page\0"
    "Setup\0"
    "Specify default setup string\0"
    "Name\0"
    "Change current printer device\0"
    "Return to previous menu\0"
    "International\0"
    "Punctuation  Currency  Date  Time  Negative  Quit\0"
    "Help\0"
    "Select Help access method\0"
    "Clock\0"
    "Standard  International  None  Clock  Filename\0"
    "Undo\0"
    "Turn undo feature on or off\0"
    "Beep\0"
    "Determine if computer bell will sound when error occurs\0"
    "Specify add-ins to be loaded automatically whenever you start 1-2-3\0"
    "Punctuation\0"
    "Specify numeric punctuation\0"
    "Specify currency sign\0"
    "Specify date formats D4 and D5\0"
    "Time\0"
    "Specify time formats D8 and D9\0"
    "Negative\0"
    "Specify display of negative numbers in , (Comma) and Currency formats\0"
    "Return to /Worksheet Global Default menu\0"
    "A (.,,)\0"
    "Decimal: Period   Argument separator: Comma       Thousands separator: Comma\0"
    "B (,..)\0"
    "Decimal: Comma    Argument separator: Period      Thousands separator: Period\0"
    "C (.;,)\0"
    "Decimal: Period   Argument separator: Semicolon   Thousands separator: Comma\0"
    "D (,;.)\0"
    "Decimal: Comma    Argument separator: Semicolon   Thousands separator: Period\0"
    "E (., )\0"
    "Decimal: Period   Argument separator: Comma       Thousands separator: Space\0"
    "F (,. )\0"
    "Decimal: Comma    Argument separator: Period      Thousands separator: Space\0"
    "G (.; )\0"
    "Decimal: Period   Argument separator: Semicolon   Thousands separator: Space\0"
    "H (,; )\0"
    "Decimal: Comma    Argument separator: Semicolon   Thousands separator: Space\0"
    "Prefix\0"
    "Currency sign precedes value\0"
    "Suffix\0"
    "Currency sign follows value\0"
    "A (MM/DD/YY)\0"
    "Format D4 will be MM/DD/YY     Format D5 will be MM/DD\0"
    "B (DD/MM/YY)\0"
    "Format D4 will be DD/MM/YY     Format D5 will be DD/MM\0"
    "C (DD.MM.YY)\0"
    "Format D4 will be DD.MM.YY     Format D5 will be DD.MM\0"
    "D (YY-MM-DD)\0"
    "Format D4 will be YY-MM-DD     Format D5 will be MM-DD\0"
    "A (HH:MM:SS)\0"
    "Format D8 will be HH:MM:SS     Format D9 will be HH:MM\0"
    "B (HH.MM.SS)\0"
    "Format D8 will be HH.MM.SS     Format D9 will be HH.MM\0"
    "C (HH,MM,SS)\0"
    "Format D8 will be HH,MM,SS     Format D9 will be HH,MM\0"
    "D (HHhMMmSSs)\0"
    "Format D8 will be HHhMMmSSs    Format D9 will be HHhMMm\0"
    "Parentheses\0"
    "Display parentheses around negative numbers\0"
    "Sign\0"
    "Display minus sign in front of negative numbers\0"
    "Instant\0"
    "Instant access; do not remove Help Disk\0"
    "Removable\0"
    "Help Disk can be removed when Help is not in use\0"
    "Standard\0"
    "Use Lotus standard date and time: DD-MMM-YY  HH:MM AM/PM\0"
    "Use current International settings for date (D4) and time (D9)\0"
    "None\0"
    "Hide date-and-time indicator\0"
    "Display the date-and-time indicator\0"
    "Filename\0"
    "Display current file name instead of date-and-time indicator\0"
    "Turn undo on\0"
    "Turn undo off\0"
    "Computer bell will sound when an error occurs\0"
    "Computer bell will not sound when an error occurs\0"
    "Set\0"
    "Specify an add-in to be attached automatically, and attach it\0"
    "Cancel setting for an auto-attach add-in, and detach it\0"
    "Run autoexecute macro when file is retrieved\0"
    "Do not run autoexecute macro when file is retrieved\0"
    "Printer automatically issues a line feed after a carriage return\0"
    "Printer does not automatically issue a line feed after a carriage return\0"
    "Do not wait at the end of a page for paper change\0"
    "Wait at the end of a page for paper change\0"
    "Fixed  Sci  Currency  ,  General  +/-  Percent  Date  Text  Hidden  Reset\0"
    "Label\0"
    "Select alignment for a label or range of labels\0"
    "Erase a cell or range of cells\0"
    "Create  Delete  Labels  Reset  Table\0"
    "Justify\0"
    "Adjust a column of labels to a specified width\0"
    "Prot\0"
    "Prevent changes to a range if global protection is on\0"
    "Unprot\0"
    "Allow change to a range when global protection is on\0"
    "Input\0"
    "Restrict data entry to unprotected cells\0"
    "Value\0"
    "Copy a range, converting formulas to values\0"
    "Trans\0"
    "Copy a range, switching columns and rows\0"
    "Search\0"
    "Find or replace a specified string in a range\0"
    "Reset\0"
    "Return specified cells to global cell format\0"
    "1 (DD-MMM-YY)\0"
    "Lotus standard long form\0"
    "2 (DD-MMM)\0"
    "Lotus standard short form\0"
    "3 (MMM-YY)\0"
    "\0"
    "4 (Long Intn'l)\0"
    "Currently configured: MM/DD/YY\0"
    "5 (Short Intn'l)\0"
    "Currently configured: MM/DD\0"
    "Time formats\0"
    "1 (HH:MM:SS AM/PM)\0"
    "2 (HH:MM AM/PM)\0"
    "3 (Long Intn'l)\0"
    "Currently configured: HH:MM:SS (24 hour)\0"
    "4 (Short Intn'l)\0"
    "Currently configured: HH:MM (24 hour)\0"
    "Create\0"
    "Create or modify a range name\0"
    "Delete a range name\0"
    "Labels\0"
    "Create range names from a range of labels\0"
    "Delete all range names\0"
    "Table\0"
    "Create a table of range names\0"
    "Each label in range names cell to its right\0"
    "Down\0"
    "Each label in range names cell below it\0"
    "Each label in range names cell to its left\0"
    "Up\0"
    "Each label in range names cell above it\0"
    "Formulas\0"
    "Search for string in formulas only\0"
    "Search for string in labels only\0"
    "Search for string in formulas and labels\0"
    "Find\0"
    "Highlight search string in the search range\0"
    "Replace\0"
    "Replace each occurrence of search string with specified text\0"
    "Retrieve\0"
    "Erase the current worksheet from memory and display the selected worksheet\0"
    "Save\0"
    "Store the entire worksheet in a worksheet file\0"
    "Combine\0"
    "Incorporate all or part of a worksheet file into the current worksheet\0"
    "Xtract\0"
    "Save a specified range in a worksheet file\0"
    "Erase a file from disk\0"
    "List\0"
    "Display the names of files in the current directory\0"
    "Import\0"
    "Read text or numbers from a text file into the worksheet\0"
    "Display and/or change the current directory\0"
    "Admin\0"
    "Reservation  Table  Link-Refresh\0"
    "Copy data from a file on disk to the worksheet\0"
    "Add\0"
    "Add values from a file on disk to values in the worksheet\0"
    "Subtract\0"
    "Subtract values from a file on disk from values in the worksheet\0"
    "Entire-File\0"
    "Incorporate entire file into worksheet\0"
    "Named/Specified-Range\0"
    "Incorporate a range from a file into the worksheet\0"
    "Save data including formulas\0"
    "Values\0"
    "Save current values and labels\0"
    "Erase a worksheet file\0"
    "Erase a print file\0"
    "Erase a graph file\0"
    "Erase any file\0"
    "List worksheet files\0"
    "List print files\0"
    "List graph files\0"
    "List all files\0"
    "Linked\0"
    "List all files linked to worksheet\0"
    "Import each line of data as a single label\0"
    "Numbers\0"
    "Import numbers and quoted text into separate columns\0"
    "Reservation\0"
    "Get or release the current file's reservation\0"
    "Enter a table of file information in the worksheet\0"
    "Link-Refresh\0"
    "Update linked cells\0"
    "Get\0"
    "Secure a reservation for saving the file\0"
    "Release\0"
    "Release a reservation for saving the file\0"
    "Send print output directly to a printer\0"
    "Send print output to a text file\0"
    "Specify a range to print\0"
    "Line\0"
    "Advance paper one line\0"
    "Advance paper to top of next page\0"
    "Options\0"
    "Header  Footer  Margins  Borders  Setup  Pg-Length  Other  Quit\0"
    "All  Range  Borders  Format\0"
    "Align\0"
    "Reset to top of page (after adjusting paper)\0"
    "Go\0"
    "Print the specified range\0"
    "Header\0"
    "Create a header\0"
    "Footer\0"
    "Create a footer\0"
    "Margins\0"
    "Left  Right  Top  Bottom  None\0"
    "Borders\0"
    "Print border columns and/or rows\0"
    "Enter printer setup string\0"
    "Specify number of lines per page\0"
    "As-Displayed  Cell-Formulas  Formatted  Unformatted\0"
    "Set left margin\0"
    "Set right margin\0"
    "Set top margin\0"
    "Bottom\0"
    "Set bottom margin\0"
    "Clear all margin settings\0"
    "Columns\0"
    "Print border columns to the left of each print range\0"
    "Rows\0"
    "Print border rows above each print range\0"
    "As-Displayed\0"
    "Print range as displayed\0"
    "Cell-Formulas\0"
    "List entries, one per line\0"
    "Formatted\0"
    "Print headers, footers, and page breaks\0"
    "Unformatted\0"
    "Do not print headers, footers, and page breaks\0"
    "All\0"
    "Return all print settings to defaults\0"
    "Clear current print range\0"
    "Clear border column and row ranges\0"
    "Return margins, page length, and setup string to defaults\0"
    "Type\0"
    "Line  Bar  XY  Stack-Bar  Pie\0"
    "X\0"
    "Set X data range\0"
    "A\0"
    "Set first data range\0"
    "B\0"
    "Set second data range\0"
    "C\0"
    "Set third data range\0"
    "D\0"
    "Set fourth data range\0"
    "E\0"
    "Set fifth data range\0"
    "F\0"
    "Set sixth data range\0"
    "Graph  X  A  B  C  D  E  F  Ranges  Options  Quit\0"
    "View\0"
    "View the current graph\0"
    "Save the current graph in a file for printing\0"
    "Legend  Format  Titles  Grid  Scale  Color  B&W  Data-Labels  Quit\0"
    "Use  Create  Delete  Reset  Table\0"
    "Group\0"
    "Set all data ranges at once\0"
    "Line graph\0"
    "Bar\0"
    "Bar graph\0"
    "XY\0"
    "XY graph\0"
    "Stack-Bar\0"
    "Stacked bar graph\0"
    "Pie\0"
    "Pie chart\0"
    "Use\0"
    "Make a named graph current\0"
    "Name the current graph\0"
    "Delete a named graph\0"
    "Delete all named graphs\0"
    "Create a table of named graphs\0"
    "Use columns as data ranges\0"
    "Use rows as data ranges\0"
    "Fill\0"
    "Fill a range with a sequence of values\0"
    "Create a table of values\0"
    "Sort\0"
    "Sort database records\0"
    "Query\0"
    "Find all records that satisfy given criteria\0"
    "Distribution\0"
    "Calculate frequency distribution of the values in a range\0"
    "Matrix\0"
    "Multiply and invert matrices\0"
    "Regression\0"
    "Calculate linear regression\0"
    "Parse\0"
    "Convert a column of long labels into a range of labels or numbers\0"
    "1\0"
    "One input cell, one or more dependent formulas\0"
    "2\0"
    "Two input cells, one dependent formula\0"
    "Clear table ranges and input cells for all data tables\0"
    "Data-Range\0"
    "Select records to be sorted\0"
    "Primary-Key\0"
    "Specify primary order for records\0"
    "Secondary-Key\0"
    "Specify order for records with same primary key\0"
    "Clear data range and sort keys\0"
    "Sort data and return to READY mode\0"
    "Specify range that contains records to search\0"
    "Criteria\0"
    "Specify the range that contains criteria\0"
    "Output\0"
    "Specify the range to which extracted records are copied\0"
    "Highlight each record that matches criteria\0"
    "Extract\0"
    "Copy all records that match criteria to output range\0"
    "Unique\0"
    "Copy records that match criteria to output range, eliminating duplicates\0"
    "Delete all records that match criteria\0"
    "Clear input, criteria, and output ranges\0"
    "Invert\0"
    "Create the inverse of a matrix\0"
    "Multiply\0"
    "Multiply two ranges as matrices\0"
    "X-Range\0"
    "Specify independent variables (X range)\0"
    "Y-Range\0"
    "Specify dependent variable (Y range)\0"
    "Output-Range\0"
    "Specify the output range\0"
    "Intercept\0"
    "Compute  Zero\0"
    "Clear the X range, Y range, output range, and reset Intercept to Compute\0"
    "Calculate a data regression on specified ranges\0"
    "Compute\0"
    "Calculate the y-axis intercept automatically\0"
    "Use zero as the y-axis intercept\0"
    "Format-Line\0"
    "Create or edit a format line at the current cell\0"
    "Input-Column\0"
    "Specify a column of labels to parse\0"
    "Specify the range in which parsed data is placed\0"
    "Clear input column and output range\0"
    "Parse labels in the input column and place them in the output range\0"
    "Create a format line at the current cell\0"
    "Edit\0"
    "Edit a format line at the current cell\0"
    "Attach\0"
    "Load an add-in program into memory\0"
    "Detach\0"
    "Remove an attached add-in program from memory\0"
    "Invoke\0"
    "Activate an attached add-in program\0"
    "Remove all attached add-in programs from memory\0"
    "Do not end 1-2-3 session; return to READY mode\0"
    "End 1-2-3 session (Remember to save your worksheet first)\0";
static const uint16_t menu_template_offsets[] = {
    0,10,85,91,170,175,205,210,240,245,316,322,368,374,447,452,
    518,525,574,581,635,640,662,669,746,753,786,793,827,834,886,892,
    931,938,972,979,1023,1030,1057,1062,1129,1135,1170,1177,1244,1257,1284,1297,
    1321,1335,1394,1405,1441,1449,1507,1512,1575,1581,1619,1623,1653,1662,1690,1692,
    1716,1724,1758,1762,1803,1811,1834,1839,1861,1866,1899,1906,1935,1940,1967,1973,
    2001,2008,2031,2039,2068,2079,2108,2116,2139,2149,2187,2194,2234,2244,2283,2290,
    2319,2327,2357,2422,2426,2479,2506,2530,2540,2573,2585,2630,2635,2659,2667,2701,
    2714,2753,2793,2842,2845,2894,2898,2903,2973,2984,3023,3032,3083,3089,3125,3174,
    3224,3229,3262,3269,3298,3328,3379,3386,3429,3467,3475,3522,3532,3558,3587,3594,
    3642,3648,3695,3704,3752,3773,3783,3809,3816,3850,3874,3899,3903,3926,3930,3956,
    3966,4003,4008,4050,4056,4085,4090,4120,4144,4158,4208,4213,4239,4245,4292,4297,
    4325,4330,4386,4454,4466,4494,4516,4547,4552,4583,4592,4662,4703,4711,4788,4796,
    4874,4882,4959,4967,5045,5053,5130,5138,5215,5223,5300,5308,5385,5392,5421,5428,
    5456,5469,5524,5537,5592,5605,5660,5673,5728,5741,5796,5809,5864,5877,5932,5946,
    6002,6014,6058,6063,6111,6119,6159,6169,6218,6227,6284,6347,6352,6381,6417,6426,
    6487,6500,6514,6560,6610,6614,6676,6732,6777,6829,6894,6967,7017,7060,7134,7140,
    7188,7219,7256,7264,7311,7316,7370,7377,7430,7436,7477,7483,7527,7533,7574,7581,
    7627,7633,7678,7692,7717,7728,7754,7765,7766,7782,7813,7830,7858,7871,7890,7906,
    7922,7963,7980,8018,8025,8055,8075,8082,8124,8147,8153,8183,8227,8232,8272,8315,
    8318,8358,8367,8402,8435,8476,8481,8525,8533,8594,8603,8678,8683,8730,8738,8809,
    8816,8859,8882,8887,8939,8946,9003,9047,9053,9086,9133,9137,9195,9204,9269,9281,
    9320,9342,9393,9422,9429,9460,9483,9502,9521,9536,9557,9574,9591,9606,9613,9648,
    9691,9699,9752,9764,9810,9861,9874,9894,9898,9939,9947,9989,10029,10062,10087,10092,
    10115,10149,10157,10221,10249,10255,10300,10303,10329,10336,10352,10359,10375,10383,10414,10422,
    10455,10482,10515,10567,10583,10600,10615,10622,10640,10666,10674,10727,10732,10773,10786,10811,
    10825,10852,10862,10902,10914,10961,10965,11003,11029,11064,11122,11127,11157,11159,11176,11178,
    11199,11201,11223,11225,11246,11248,11270,11272,11293,11295,11316,11366,11371,11394,11440,11507,
    11541,11547,11575,11586,11590,11600,11603,11612,11622,11640,11644,11654,11658,11685,11708,11729,
    11753,11784,11811,11835,11840,11879,11904,11909,11931,11937,11982,11995,12053,12060,12089,12100,
    12128,12134,12200,12202,12249,12251,12290,12345,12356,12384,12396,12430,12444,12492,12523,12558,
    12604,12613,12654,12661,12717,12761,12769,12822,12829,12902,12941,12982,12989,13020,13029,13061,
    13069,13109,13117,13154,13167,13192,13202,13216,13289,13337,13345,13390,13423,13435,13484,13497,
    13533,13582,13618,13686,13727,13732,13771,13778,13813,13820,13866,13873,13909,13957,14004,
};
static const i123_menu_item_t menu_template_items[] = {
    {0,1,1},
    {2,3,32},
    {4,5,65535},
    {6,7,65535},
    {8,9,43},
    {10,11,54},
    {12,13,67},
    {14,15,71},
    {16,17,65535},
    {18,19,80},
    {20,21,81},
    {22,23,2},
    {24,25,7},
    {26,27,8},
    {28,29,9},
    {30,31,11},
    {32,33,12},
    {34,35,13},
    {36,37,65535},
    {38,39,65535},
    {40,41,14},
    {42,43,3},
    {44,45,4},
    {46,47,65535},
    {48,49,5},
    {50,51,6},
    {52,53,15},
    {54,55,65535},
    {56,57,65535},
    {58,59,65535},
    {60,61,65535},
    {62,63,65535},
    {64,65,65535},
    {66,67,65535},
    {68,69,65535},
    {70,71,36},
    {72,73,65535},
    {74,75,65535},
    {76,77,65535},
    {78,79,65535},
    {80,81,65535},
    {82,83,65535},
    {84,85,65535},
    {86,87,65535},
    {88,89,65535},
    {90,91,65535},
    {92,93,65535},
    {94,95,65535},
    {96,97,65535},
    {28,98,65535},
    {99,100,65535},
    {28,101,65535},
    {99,102,65535},
    {103,104,65535},
    {105,106,65535},
    {107,108,65535},
    {109,110,65535},
    {111,112,10},
    {103,113,65535},
    {105,114,65535},
    {115,116,65535},
    {117,31,65535},
    {118,119,65535},
    {120,121,65535},
    {122,123,65535},
    {124,125,65535},
    {120,126,65535},
    {122,127,65535},
    {128,129,65535},
    {130,131,65535},
    {124,132,65535},
    {2,133,65535},
    {134,135,65535},
    {30,136,65535},
    {137,138,16},
    {139,140,65535},
    {36,141,65535},
    {142,143,65535},
    {144,145,17},
    {146,147,29},
    {20,148,65535},
    {149,150,65535},
    {151,152,30},
    {76,153,65535},
    {78,154,65535},
    {155,156,65535},
    {157,158,65535},
    {159,160,65535},
    {161,162,31},
    {163,164,65535},
    {165,166,65535},
    {20,167,65535},
    {168,169,18},
    {170,171,24},
    {172,173,25},
    {174,175,26},
    {176,177,27},
    {18,178,28},
    {179,180,19},
    {60,181,20},
    {70,182,21},
    {183,184,22},
    {185,186,23},
    {20,187,65535},
    {188,189,65535},
    {190,191,65535},
    {192,193,65535},
    {194,195,65535},
    {196,197,65535},
    {198,199,65535},
    {200,201,65535},
    {202,203,65535},
    {204,205,65535},
    {206,207,65535},
    {208,209,65535},
    {210,211,65535},
    {212,213,65535},
    {214,215,65535},
    {216,217,65535},
    {218,219,65535},
    {220,221,65535},
    {222,223,65535},
    {224,225,65535},
    {226,227,65535},
    {228,229,65535},
    {230,231,65535},
    {232,233,65535},
    {168,234,65535},
    {235,236,65535},
    {172,237,65535},
    {238,239,65535},
    {94,240,65535},
    {96,241,65535},
    {94,242,65535},
    {96,243,65535},
    {244,245,65535},
    {134,246,65535},
    {94,247,65535},
    {96,248,65535},
    {117,249,65535},
    {115,250,65535},
    {115,251,65535},
    {117,252,65535},
    {42,253,33},
    {254,255,38},
    {30,256,65535},
    {165,257,39},
    {258,259,65535},
    {260,261,65535},
    {262,263,65535},
    {264,265,65535},
    {266,267,65535},
    {268,269,65535},
    {270,271,41},
    {56,57,65535},
    {58,59,65535},
    {60,61,65535},
    {62,63,65535},
    {64,65,65535},
    {66,67,65535},
    {68,69,65535},
    {70,71,34},
    {72,73,65535},
    {74,75,65535},
    {272,273,65535},
    {274,275,65535},
    {276,277,65535},
    {278,279,65535},
    {280,281,65535},
    {282,283,65535},
    {183,284,35},
    {285,279,65535},
    {286,279,65535},
    {287,288,65535},
    {289,290,65535},
    {274,275,65535},
    {276,277,65535},
    {278,279,65535},
    {280,281,65535},
    {282,283,65535},
    {183,284,37},
    {285,279,65535},
    {286,279,65535},
    {287,288,65535},
    {289,290,65535},
    {76,77,65535},
    {78,79,65535},
    {80,81,65535},
    {291,292,65535},
    {26,293,65535},
    {294,295,40},
    {272,296,65535},
    {297,298,65535},
    {78,299,65535},
    {300,301,65535},
    {76,302,65535},
    {303,304,65535},
    {305,306,65535},
    {254,307,65535},
    {118,308,65535},
    {309,310,65535},
    {311,312,65535},
    {313,314,65535},
    {315,316,65535},
    {317,318,44},
    {319,320,48},
    {30,321,49},
    {322,323,50},
    {324,325,51},
    {139,326,65535},
    {327,328,52},
    {4,329,45},
    {330,331,46},
    {332,333,47},
    {334,335,65535},
    {336,337,65535},
    {334,335,65535},
    {336,337,65535},
    {334,335,65535},
    {336,337,65535},
    {305,338,65535},
    {339,340,65535},
    {0,341,65535},
    {10,342,65535},
    {12,343,65535},
    {144,344,65535},
    {0,345,65535},
    {10,346,65535},
    {12,347,65535},
    {144,348,65535},
    {349,350,65535},
    {72,351,65535},
    {352,353,65535},
    {354,355,53},
    {297,356,65535},
    {357,358,65535},
    {359,360,65535},
    {361,362,65535},
    {137,363,55},
    {8,364,61},
    {2,365,65535},
    {366,367,65535},
    {38,368,65535},
    {369,370,56},
    {124,371,60},
    {372,373,65535},
    {374,375,65535},
    {20,148,65535},
    {376,377,65535},
    {378,379,65535},
    {380,381,57},
    {382,383,58},
    {163,384,65535},
    {159,385,65535},
    {144,386,59},
    {20,167,65535},
    {76,387,65535},
    {78,388,65535},
    {155,389,65535},
    {390,391,65535},
    {235,392,65535},
    {393,394,65535},
    {395,396,65535},
    {397,398,65535},
    {399,400,65535},
    {401,402,65535},
    {403,404,65535},
    {405,406,65535},
    {2,407,65535},
    {382,408,65535},
    {42,409,65535},
    {2,365,65535},
    {366,367,65535},
    {38,368,65535},
    {369,370,62},
    {124,371,66},
    {372,373,65535},
    {374,375,65535},
    {20,148,65535},
    {376,377,65535},
    {378,379,65535},
    {380,381,63},
    {382,383,64},
    {163,384,65535},
    {159,385,65535},
    {144,386,65},
    {20,167,65535},
    {76,387,65535},
    {78,388,65535},
    {155,389,65535},
    {390,391,65535},
    {235,392,65535},
    {393,394,65535},
    {395,396,65535},
    {397,398,65535},
    {399,400,65535},
    {401,402,65535},
    {403,404,65535},
    {405,406,65535},
    {2,407,65535},
    {382,408,65535},
    {42,409,65535},
    {410,411,68},
    {412,413,65535},
    {414,415,65535},
    {416,417,65535},
    {418,419,65535},
    {420,421,65535},
    {422,423,65535},
    {424,425,65535},
    {272,426,65535},
    {427,428,65535},
    {315,429,65535},
    {369,430,65535},
    {165,431,69},
    {432,433,70},
    {20,148,65535},
    {366,434,65535},
    {435,436,65535},
    {437,438,65535},
    {439,440,65535},
    {441,442,65535},
    {443,444,65535},
    {291,445,65535},
    {26,446,65535},
    {272,447,65535},
    {297,448,65535},
    {84,449,65535},
    {86,450,65535},
    {451,452,65535},
    {297,453,72},
    {454,455,73},
    {456,457,74},
    {458,459,65535},
    {460,461,75},
    {462,463,76},
    {464,465,78},
    {466,467,65535},
    {468,469,65535},
    {272,470,65535},
    {471,472,65535},
    {473,474,65535},
    {475,476,65535},
    {272,477,65535},
    {374,478,65535},
    {20,148,65535},
    {264,479,65535},
    {480,481,65535},
    {482,483,65535},
    {309,484,65535},
    {485,486,65535},
    {487,488,65535},
    {26,489,65535},
    {272,490,65535},
    {20,148,65535},
    {491,492,65535},
    {493,494,65535},
    {495,496,65535},
    {497,498,65535},
    {499,500,65535},
    {501,502,77},
    {272,503,65535},
    {374,504,65535},
    {20,148,65535},
    {505,506,65535},
    {54,507,65535},
    {508,509,79},
    {510,511,65535},
    {499,512,65535},
    {272,513,65535},
    {374,514,65535},
    {20,148,65535},
    {291,515,65535},
    {516,517,65535},
    {518,519,65535},
    {520,521,65535},
    {522,523,65535},
    {124,524,65535},
    {20,148,65535},
    {115,525,65535},
    {117,526,65535},
};
static const i123_menu_level_t menu_template_levels[] = {
    {0,11},
    {11,10},
    {21,7},
    {28,10},
    {38,3},
    {41,6},
    {47,2},
    {49,2},
    {51,2},
    {53,5},
    {58,2},
    {60,2},
    {62,4},
    {66,5},
    {71,3},
    {74,7},
    {81,11},
    {92,6},
    {98,6},
    {104,8},
    {112,2},
    {114,4},
    {118,4},
    {122,2},
    {124,2},
    {126,5},
    {131,2},
    {133,2},
    {135,2},
    {137,2},
    {139,2},
    {141,2},
    {143,11},
    {154,11},
    {165,6},
    {171,4},
    {175,6},
    {181,4},
    {185,3},
    {188,5},
    {193,4},
    {197,3},
    {200,2},
    {202,9},
    {211,3},
    {214,2},
    {216,2},
    {218,2},
    {220,2},
    {222,4},
    {226,5},
    {231,2},
    {233,3},
    {236,2},
    {238,2},
    {240,8},
    {248,8},
    {256,5},
    {261,2},
    {263,4},
    {267,4},
    {271,8},
    {279,8},
    {287,5},
    {292,2},
    {294,4},
    {298,4},
    {302,15},
    {317,5},
    {322,5},
    {327,2},
    {329,8},
    {337,3},
    {340,6},
    {346,9},
    {355,2},
    {357,7},
    {364,2},
    {366,6},
    {372,2},
    {374,5},
    {379,2},
};
#ifdef I123_MENU_HOST
const char *const i123_menu_paths[] = {
"/",
"/Worksheet",
"/Worksheet/Global",
"/Worksheet/Global/Format",
"/Worksheet/Global/Label-Prefix",
"/Worksheet/Global/Recalculation",
"/Worksheet/Global/Protection",
"/Worksheet/Insert",
"/Worksheet/Delete",
"/Worksheet/Column",
"/Worksheet/Column/Column-Range",
"/Worksheet/Erase",
"/Worksheet/Titles",
"/Worksheet/Window",
"/Worksheet/Learn",
"/Worksheet/Global/Default",
"/Worksheet/Global/Default/Printer",
"/Worksheet/Global/Default/Other",
"/Worksheet/Global/Default/Other/International",
"/Worksheet/Global/Default/Other/International/Punctuation",
"/Worksheet/Global/Default/Other/International/Currency",
"/Worksheet/Global/Default/Other/International/Date",
"/Worksheet/Global/Default/Other/International/Time",
"/Worksheet/Global/Default/Other/International/Negative",
"/Worksheet/Global/Default/Other/Help",
"/Worksheet/Global/Default/Other/Clock",
"/Worksheet/Global/Default/Other/Undo",
"/Worksheet/Global/Default/Other/Beep",
"/Worksheet/Global/Default/Other/Add-In",
"/Worksheet/Global/Default/Autoexec",
"/Worksheet/Global/Default/Printer/AutoLF",
"/Worksheet/Global/Default/Printer/Wait",
"/Range",
"/Range/Format",
"/Range/Format/Date",
"/Range/Format/Date/Time",
"/Worksheet/Global/Format/Date",
"/Worksheet/Global/Format/Date/Time",
"/Range/Label",
"/Range/Name",
"/Range/Name/Labels",
"/Range/Search",
"/Range/Search/Find",
"/File",
"/File/Combine",
"/File/Combine/Copy",
"/File/Combine/Add",
"/File/Combine/Subtract",
"/File/Xtract",
"/File/Erase",
"/File/List",
"/File/Import",
"/File/Admin",
"/File/Admin/Reservation",
"/Print",
"/Print/Printer",
"/Print/Printer/Options",
"/Print/Printer/Options/Margins",
"/Print/Printer/Options/Borders",
"/Print/Printer/Options/Other",
"/Print/Printer/Clear",
"/Print/File",
"/Print/File/Options",
"/Print/File/Options/Margins",
"/Print/File/Options/Borders",
"/Print/File/Options/Other",
"/Print/File/Clear",
"/Graph",
"/Graph/Type",
"/Graph/Name",
"/Graph/Group",
"/Data",
"/Data/Table",
"/Data/Sort",
"/Data/Query",
"/Data/Matrix",
"/Data/Regression",
"/Data/Regression/Intercept",
"/Data/Parse",
"/Data/Parse/Format-Line",
"/Add-In",
"/Quit",
};
#endif
