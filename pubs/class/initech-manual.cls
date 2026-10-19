%% Initech Publications house class. Binding reference: PUB-REF-0001,
%% docs/pubs/DESIGN-REFERENCE.md, especially PG-01..35, TY-01..21, PK-02,
%% PN-01..06 and Sec 11.5. LuaLaTeX is the D-11 document exception.
%% Options: ibm (default), ashtontate, lotus, wordperfect, borland, apple,
%% operations; binder (same 22 mm inside margin); cover; spine; final.
%% The physical block is 142 x 186.3 mm, margins 22/13.8 mm, columns
%% 34 + 4 + 104 mm, fixed left on BOTH pages. Lotus uses 132 mm measure.
%% All font files are pinned in pubs/fonts.lock. PDF optional dates and ID
%% are suppressed; test-pubs-repro compares independent clean builds.
%% F-IBM furniture (option ibm; PG-10, PG-20, PG-28, PG-30, PG-35; merged
%% from the InitechDOS manuals lane, bead initech-ipu5.7): "Chapter N. Title"
%% openers over the AN-22 mini-contents (titletoc partial contents), a
%% "Notes:" verso behind every opener, black thumb tabs on every recto of a
%% chapter stepping down the fore-edge (six positions, counted through the
%% book; label from \ChapterTab{} before \chapter, else "Chapter N"), folio
%% only in the foot, index folios Index-n, and the two-line title of a
%% reference entry repeated flush outside on every continuation page (PG-10,
%% drawn at shipout for every family: \im@openentry / \im@closeentry).
%% IDENTITY (PUB-REF-0001 D-8 revised, Sec 7.0, PK-01, AN-27; WL-0132).
%% Stable public names -- other manual lanes call them:
%%   \InitechLogo[<height>]       the Initech logo, colour (colour pieces only)
%%   \InitechLogoBlack[<height>]  the logo in black + black tints (interiors)
%%   \InitechMark[<h>], \InitechMarkBlack[<h>]   the three blocks alone
%%   \InitechLogotype[<cap height>]  the INITECH letters, current colour
%%   \InitechLogoForWidth{<w>}    the lockup height giving width <w>
%%   \Wordmark[<width>]           kept name: the logo at <width>, colour on
%%                                colour pieces, black inside a book
%%   \IsThisGoodForTheCompany[<width>]  the company banner, one display
%%                                line (default \linewidth); COMPANY cyan on
%%                                colour pieces, black inside (D-6)
%%   \MakeCover, \MakeBackCover, \MakeSpine, \MakeTitlePage
%%   \ColourPiece                 mark a document as a colour piece (box,
%%                                label, card) so \Wordmark and the banner
%%                                print in colour; covers and spines are
%%                                colour pieces by their class option
%%   colours: InitechCyan (the line colour), InitechCubeLight,
%%   InitechCubeMono, InitechBlack, InitechPutty, Family*.
%% The logo drawing lives in pubs/common/initech-logo.tex (TEXINPUTS must
%% include pubs/common, as pubs/build.sh and the harness scripts do).
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{initech-manual}[1992/03/01 Initech manual class]

\RequirePackage{iftex}
\RequireLuaTeX

\newif\ifim@continuous
\newif\ifim@binder
\newif\ifim@final
\newif\ifim@cover
\newif\ifim@spine
%% PUB-REF-0001 PG-03..27, TY-01: model family selects the interior.
\def\im@model{ibm}
\newcommand\im@ifmodel[3]{\edef\im@wanted{#1}%
  \ifx\im@model\im@wanted #2\else #3\fi}
\DeclareOption{ibm}{\def\im@model{ibm}}
\DeclareOption{ashtontate}{\def\im@model{ashtontate}}
\DeclareOption{lotus}{\def\im@model{lotus}}
\DeclareOption{wordperfect}{\def\im@model{wordperfect}}
\DeclareOption{borland}{\def\im@model{borland}}
\DeclareOption{apple}{\def\im@model{apple}}
\DeclareOption{operations}{\def\im@model{operations}}
\DeclareOption{chapterfolio}{\im@continuousfalse}
\DeclareOption{continuous}{\im@continuoustrue}
\DeclareOption{binder}{\im@bindertrue}
\DeclareOption{final}{\im@finaltrue}
\DeclareOption{cover}{\im@covertrue}
\DeclareOption{spine}{\im@spinetrue}
%% quickref: the PK-11 Quick Reference booklet, 88.9 x 228.6 mm (the house
%% trim halved), saddle-stitched; single column (bead initech-ipu5.8).
\newif\ifim@quickref
\DeclareOption{quickref}{\im@quickreftrue}
\DeclareOption*{\ClassError{initech-manual}{Unknown option `\CurrentOption'}{}}
\ProcessOptions\relax
\im@ifmodel{apple}{\im@continuoustrue}{}
\im@ifmodel{borland}{\im@continuoustrue}{}
\im@ifmodel{wordperfect}{\im@continuoustrue}{}
\newif\ifim@ibm
\im@ifmodel{ibm}{\im@ibmtrue}{}

\LoadClass[10pt,twoside,openright]{book}

%% ---- reproducible PDF (Rule 11) -----------------------------------------
%% 1 FullBanner, 2 FileName, 4 PageNumber, 8 InfoDict, 16 Creator,
%% 32 CreationDate, 64 ModDate, 128 Producer, 256 Trapped, 512 ID.
\ifdefined\initechpubsmutant
  %% MUTANT (make test-pubs-repro-mutant only): put the build time into the
  %% document and keep the PDF dates/ID, so two builds must differ.
  \xdef\im@buildtime{\directlua{tex.sprint(tostring(os.time()) .. " " .. tostring(os.clock()))}}
  \AddToHook{shipout/foreground}{\put(20,-20){\tiny build \im@buildtime}}
\else
  \pdfvariable suppressoptionalinfo \numexpr 1+2+4+8+16+32+64+128+256+512\relax
\fi
\pdfvariable compresslevel 9
\pdfvariable objcompresslevel 2

%% ---- fonts (PUB-REF-0001 TY-01..21) ---------------
\RequirePackage{fontspec}
\defaultfontfeatures{Ligatures=TeX}
\def\im@bodyfont{texgyrepagella}
\def\im@bodysize{10}
\newcommand\im@timesbody{\def\im@bodyfont{texgyretermes}\def\im@bodysize{10.5}}
\im@ifmodel{ibm}{\im@timesbody}{}
\im@ifmodel{operations}{\im@timesbody}{}
\im@ifmodel{wordperfect}{\im@timesbody}{}
\setmainfont{\im@bodyfont}[
  Extension=.otf, UprightFont=*-regular, BoldFont=*-bold,
  ItalicFont=*-italic, BoldItalicFont=*-bolditalic]
\setsansfont{texgyreheros}[
  Extension=.otf, UprightFont=*-regular, BoldFont=*-bold,
  ItalicFont=*-italic, BoldItalicFont=*-bolditalic]
\setmonofont{texgyrecursor}[
  Extension=.otf, UprightFont=*-regular, BoldFont=*-bold,
  ItalicFont=*-italic, BoldItalicFont=*-bolditalic, HyphenChar=None]
\newfontfamily\im@condensed{texgyreheroscn}[
  Extension=.otf, UprightFont=*-regular, BoldFont=*-bold,
  ItalicFont=*-italic, BoldItalicFont=*-bolditalic]
\newfontfamily\im@termes{texgyretermes}[
  Extension=.otf, UprightFont=*-regular, BoldFont=*-bold,
  ItalicFont=*-italic, BoldItalicFont=*-bolditalic]
\newfontfamily\im@wordmarkfont{texgyreheros}[
  Extension=.otf, UprightFont=*-bold, LetterSpace=10]
\newfontfamily\im@display{EBGaramond-Regular.otf}
\newfontfamily\im@adventor{texgyreadventor}[
  Extension=.otf, UprightFont=*-regular, ItalicFont=*-italic,
  BoldFont=*-bold, BoldItalicFont=*-bolditalic]

%% Type sizes.  Body 10/13 (Sec 4).
\renewcommand\normalsize{\@setfontsize\normalsize{\im@bodysize}{12}%
  \abovedisplayskip 6.5pt \belowdisplayskip 6.5pt
  \abovedisplayshortskip 0pt \belowdisplayshortskip 6.5pt}
\renewcommand\small{\@setfontsize\small{9}{11}}
\renewcommand\footnotesize{\@setfontsize\footnotesize{8}{10}}
\renewcommand\scriptsize{\@setfontsize\scriptsize{6.5}{8}}
\normalsize
\newcommand\imChapterTitleFont{\sffamily\bfseries\@setfontsize\im@x{24}{28}}
\newcommand\imChapterNumberFont{\sffamily\bfseries\@setfontsize\im@x{10}{13}}
\newcommand\imHeadOneFont{\sffamily\bfseries\@setfontsize\im@x{13}{16}}
\newcommand\imHeadTwoFont{\sffamily\bfseries\@setfontsize\im@x{10.5}{13}}
\newcommand\imSideHeadFont{\im@condensed\bfseries\@setfontsize\im@x{9}{11}}
\newcommand\imFootFont{\sffamily\@setfontsize\im@x{8}{10}}
\newcommand\imFolioFont{\sffamily\bfseries\fontsize{9}{10}\selectfont}
\newcommand\imCaptionFont{\sffamily\@setfontsize\im@x{8}{10}}
\newcommand\imLegalFont{\sffamily\@setfontsize\im@x{6.5}{8}}
\newcommand\imTypeFont{\ttfamily\@setfontsize\im@x{9}{12}}

%% ---- page grid (PUB-REF-0001 PG-01..09) --------------------------------------
\newlength\imSideCol   \setlength\imSideCol{34mm}
\newlength\imSideGap   \setlength\imSideGap{4mm}
\newlength\imHang      \setlength\imHang{38mm}
\newlength\imFullWidth \setlength\imFullWidth{142mm}
\newlength\imInside    \setlength\imInside{22mm}
\newlength\imOutside   \setlength\imOutside{13.8mm}
\im@ifmodel{lotus}{\setlength\imHang{0mm}}{}
\newlength\imTextWidth\setlength\imTextWidth{\dimexpr\imFullWidth-\imHang\relax}
\im@ifmodel{lotus}{\setlength\imTextWidth{132mm}}{}

\RequirePackage{calc}
\RequirePackage{changepage}
\RequirePackage{geometry}
\ifim@spine
  \providecommand\SpineWidth{44mm}
  \geometry{paperwidth=\SpineWidth, paperheight=9in, margin=0pt,
    noheadfoot, nomarginpar}
\else\ifim@quickref
  \geometry{paperwidth=3.5in, paperheight=9in, twoside,
    inner=9mm, outer=7mm, top=14mm, textheight=200mm,
    footskip=8mm, headheight=10pt, headsep=3mm, nomarginpar}
\else\ifim@cover
  \geometry{paperwidth=7in, paperheight=9in, margin=0pt,
    noheadfoot, nomarginpar}
\else
  \geometry{paperwidth=7in, paperheight=9in, twoside,
    textwidth=\imTextWidth, top=19mm,
    textheight=186.3mm,
    footskip=14.3mm, headheight=10pt, headsep=3mm,
    inner=\imInside+\imHang, nomarginpar}
  %% Side-head column on the LEFT of both pages: on a verso the left (outside)
  %% margin is 13.8 mm + 38 mm.  geometry mirrors margins, so set the verso
  %% offset ourselves once geometry has done its work.
  \AtBeginDocument{%
    \setlength\evensidemargin{\imOutside+\imHang-1in}%
    \setlength\oddsidemargin{\imInside+\imHang-1in}}
\fi\fi\fi
\setlength\topskip{10pt}
\setlength\parindent{0pt}
\setlength\parskip{6.5pt plus 1pt}
\im@ifmodel{ibm}{\setlength\parskip{12pt}}{\setlength\parskip{6pt}}
\RequirePackage{ragged2e}
\setlength\RaggedRightRightskip{0pt plus 4em}
\AtBeginDocument{\RaggedRight}
\clubpenalty=10000 \widowpenalty=10000 \displaywidowpenalty=10000
\emergencystretch=1em
\frenchspacing
\raggedbottom

%% ---- colour (D-6: black interior; colour only on covers, spines, tabs) ----
\RequirePackage[cmyk]{xcolor}
\definecolor{InitechBlack}{cmyk}{0,0,0,1}
\definecolor{RuleGrey}{cmyk}{0,0,0,0.5}
\definecolor{InitechPutty}{cmyk}{0,0.03,0.12,0.10}
\definecolor{FamilySystems}{cmyk}{1,0,0.45,0.35}
\definecolor{FamilyInformation}{cmyk}{1,0.67,0,0.23}
\definecolor{FamilyDecision}{cmyk}{0.90,0.12,0.95,0.40}
\definecolor{FamilyOffice}{cmyk}{0,1,0.63,0.29}
\definecolor{FamilyProgramming}{cmyk}{0.90,1,0,0.15}
%% Initech identity (PUB-REF-0001 D-8 as revised by operator directive, PK-01;
%% worklog WL-0132).  InitechCyan is THE Initech identity
%% colour, measured on the logo's cube tops (RGB 87/177/159, #57B19F); the
%% press build 62/0/34/8 is the CMYK whose proof rendering is nearest that
%% sample (PK-01 shows the arithmetic).  It is the LINE colour of every
%% colour piece (logo, title hairline, foot line band, slogan); family
%% colours keep the family band.  Never inside a book (D-6): interiors use
%% the one-colour logo, whose cube tops are InitechCubeMono (the cyan's
%% luminance, 42 % black).  InitechCubeLight: the logo's light faces
%% (RGB 213/212/212 = 16 % black).
\definecolor{InitechCyan}{cmyk}{0.62,0,0.34,0.08}
\definecolor{InitechCubeLight}{cmyk}{0,0,0,0.16}
\definecolor{InitechCubeMono}{cmyk}{0,0,0,0.42}

%% ---- folios and running foot ----------------------------------------------
\RequirePackage{fancyhdr}
%% VO-30 (initech-syntax \Slip): whatever a running head or foot prints is
%% printed in the place `running-head'.
\fancyhfinit{\im@slipat{running-head}}
\newcommand\im@foottitle{}
\newcommand\im@draftfoot{}
\newif\ifim@referencepages
\newcommand\DictionaryHeads{\global\im@referencepagestrue}
\newcommand\LibraryHeads{\global\im@referencepagestrue}
\newcommand\im@folioprefix{}
\newcommand\im@sectionprefix{U}
\newcommand\SectionPrefix[1]{\gdef\im@sectionprefix{#1}}
\newcommand\im@setfolio[1]{\xdef\im@folioprefix{#1}}
\newcommand\im@folioform{\gdef\thepage{\im@folioprefix\arabic{page}}}
\im@folioform
\newcommand\FrontMatterFolios{\gdef\thepage{\roman{page}}}
\fancypagestyle{im@text}{%
  \fancyhf{}%
  \renewcommand\headrulewidth{0pt}%
  \renewcommand\footrulewidth{0pt}%
  \fancyhfoffset[LO,LE]{\imHang}%
  \fancyfoot[RO,LE]{\imFolioFont\thepage}%
  \fancyfoot[C]{\imFootFont\bfseries\im@draftfoot}%
  \im@ifmodel{ibm}{}{\fancyfoot[LO,RE]{\imFootFont\im@foottitle}}%
  \im@ifmodel{operations}{\renewcommand\headrulewidth{0.5pt}%
    \renewcommand\footrulewidth{0.5pt}%
    \fancyfoot[RE]{\imFootFont\im@opsshort}%
    \fancyfoot[LO]{\imFootFont\im@footlabel\im@foottitle}}{}%
  \im@ifmodel{ashtontate}{\renewcommand\headrulewidth{0.5pt}%
    \fancyhead[RE]{\global\im@thumbtrue\imFootFont\bfseries CHAPTER \thechapter}%
    \fancyhead[LO]{\global\im@thumbtrue\imFootFont\bfseries\MakeUppercase{\im@foottitle}}%
    \fancyhead[LE,RO]{\ifim@referencepages
      \sffamily\bfseries\fontsize{14}{16}\selectfont\MakeUppercase{\rightmark}\fi}%
    \fancyfoot[LO,RE]{\imFootFont\bfseries\ifx\im@secttitle\@empty
      \im@product\ \im@doctitle\else\MakeUppercase{\im@secttitle}\fi}}{}%
  \im@ifmodel{apple}{\fancyfoot[LO,RE]{\imFootFont Chapter \thechapter: \im@foottitle}}{}%
  \im@ifmodel{wordperfect}{\fancyfoot[LO,RE]{\imFootFont\MakeUppercase{\leftmark}}}{}%
  \im@ifmodel{lotus}{\fancyfoot[LO]{\imFootFont\rightmark}}{}%
  \im@ifmodel{borland}{\fancyhead[LE,RO]{\ifim@referencepages\imFootFont\bfseries\rightmark\fi}%
    \fancyfoot[RE]{\imFootFont\im@product\ \im@doctitle}%
    \fancyfoot[LO]{\im@adventor\itshape\footnotesize Chapter \thechapter, \im@foottitle}}{}}
\fancypagestyle{plain}{%
  \fancyhf{}%
  \renewcommand\headrulewidth{0pt}%
  \renewcommand\footrulewidth{0pt}%
  \fancyhfoffset[LO,LE]{\imHang}%
  \fancyfoot[RO,LE]{\imFolioFont\thepage}%
  \fancyfoot[C]{\imFootFont\bfseries\im@draftfoot}}
\fancypagestyle{empty}{\fancyhf{}\renewcommand\headrulewidth{0pt}%
  \renewcommand\footrulewidth{0pt}}
\pagestyle{im@text}
%% F-OPS (PG-11): the recto foot names the numbered unit in force ("Session
%% 3." / "Appendix B."); front matter, contents and index carry the title
%% alone.  \im@chapterhook sets the label after its \FootTitle (ipu5.14).
\newcommand\im@footlabel{}
%% F-OPS verso foot: the book short title (ZR23 "DOS Versions 3.3 and 4.0").
\newcommand\im@opsshort{\im@product\ \im@doctitle}
\newcommand\ShortTitle[1]{\gdef\im@opsshort{#1}}
\newcommand\FootTitle[1]{\gdef\im@foottitle{#1}\gdef\im@footlabel{}}
%% \RunningTitle{text} after \chapter: the running foot names the unit by a
%% title of its own instead of the heading (keeps the F-OPS "Session N."
%% label).  Period books pasted running heads up apart from the openers.
\newcommand\RunningTitle[1]{\gdef\im@foottitle{#1}}
%% F-ATE (PG-13): the running foot names the SECTION in force ("USING
%% INITECHBASE"), set by \BeginSection (initech-xbase.sty); empty = the book.
\newcommand\im@secttitle{}
\renewcommand\sectionmark[1]{\markright{#1}}

%% Ashton-Tate pattern: the blank verso before a recto opener is a "Notes" page.
\renewcommand\cleardoublepage{\clearpage
  \if@twoside\ifodd\c@page\else
    \im@ifmodel{borland}{\thispagestyle{im@text}}{\thispagestyle{empty}}%
    \hbox{}%
    \im@ifmodel{ibm}{\thispagestyle{im@text}\vspace*{-\topskip}\vspace*{-\parskip}%
      {\noindent\hspace*{-\imHang}\imHeadOneFont Notes:\par}}{}%
    \newpage
  \fi\fi}

%% ---- headings (Sec 4, Sec 5 chapter opener) -------------------------------
\RequirePackage[explicit]{titlesec}
\setcounter{secnumdepth}{0}
\renewcommand\thechapter{\arabic{chapter}}
\newcommand\im@chaptername{Chapter}
\newcommand\im@chapterlabel{{\imChapterNumberFont\im@chaptername\ \thechapter\par}%
  \vspace{2pt}\titlerule[2pt]}
\newcommand\im@headrag{\rightskip=0pt plus 1fil\relax\hyphenpenalty=10000\relax}
%% titlesec's chapter measure is the body measure. Make the negative hang
%% explicit in the box width, so display labels also occupy the full grid.
\newcommand\im@fullhead[1]{\hspace*{-\imHang}%
  \parbox[t]{\dimexpr\imTextWidth+\imHang\relax}{\RaggedRight #1}}
\newcommand\im@borlabel{\im@fullhead{\raggedleft\vspace*{24mm}%
  {\fontsize{11}{12}\selectfont
    \makebox[\imFullWidth][s]{\im@displayword}}\par
  \vspace{6mm}\rule{\imFullWidth}{1pt}\par\vspace{6pt}%
  {\fontsize{24}{28}\selectfont\thechapter}}}
\newcommand\im@displayword{C\hfill H\hfill A\hfill P\hfill T\hfill E\hfill R}
\titleformat{\chapter}[display]
  {\im@headrag}
  {\im@chapterlabel}
  {14pt}
  {\imChapterTitleFont #1}
  [\vspace{6pt}]
\titleformat{name=\chapter,numberless}[display]
  {\im@headrag}
  {\im@fullhead{\rule{\dimexpr\imTextWidth+\imHang\relax}{2pt}}}
  {14pt}
  {\im@fullhead{\imChapterTitleFont #1}}
  [\vspace{6pt}]
\titlespacing*{\chapter}{-\imHang}{0pt}{26pt}
\titleformat{\section}[block]{\imHeadOneFont\im@headrag}{}{0pt}{#1}
\titlespacing*{\section}{-\imHang}{19.5pt plus 4pt minus 2pt}{6.5pt}
\titleformat{\subsection}[block]{\imHeadTwoFont\im@headrag}{}{0pt}{#1}
\titlespacing*{\subsection}{0pt}{13pt plus 3pt minus 2pt}{3pt}
%% PUB-REF-0001 PG-20..26: model-specific chapter furniture.
\im@ifmodel{ibm}{%
  \titleformat{\chapter}[block]{\RaggedRight\sffamily\bfseries\fontsize{20}{24}\selectfont}
    {}{0pt}{\im@fullhead{\im@chaptername\ \thechapter.\ #1}}%
  \renewcommand\imHeadOneFont{\sffamily\bfseries\fontsize{16}{18}\selectfont}%
  \renewcommand\imHeadTwoFont{\sffamily\bfseries\fontsize{12}{14}\selectfont}%
  \titleformat{name=\chapter,numberless}[block]{\RaggedRight\sffamily\bfseries\fontsize{20}{24}\selectfont}
    {}{0pt}{\im@fullhead{#1}}%
  \titleformat{\section}[block]{\imHeadOneFont}{}{0pt}{\im@fullhead{#1}}%
  \titlespacing*{\section}{0pt}{18pt}{6pt}}{}
\im@ifmodel{operations}{%
  %% PG-21: "Session N." openers; appendices keep "Appendix A." (ipu5.14).
  \renewcommand\im@chaptername{Session}%
  \titleformat{\chapter}[display]{\RaggedRight}{\hspace*{-\imHang}\rule{40mm}{2pt}}{6pt}
    {\im@fullhead{\sffamily\bfseries\fontsize{16}{18}\selectfont \im@chaptername\ \thechapter.\ #1}}}{}
\im@ifmodel{apple}{%
  \titleformat{\chapter}[block]{\RaggedRight}{}{0pt}
    {\im@fullhead{\colorbox{black!40}{\parbox[c][25mm][c]{25mm}{\centering\color{white}%
      \sffamily\bfseries\fontsize{36}{36}\selectfont\thechapter}}\quad
      \parbox[c]{100mm}{\im@condensed\bfseries\fontsize{20}{24}\selectfont #1}}}}{}
\im@ifmodel{ashtontate}{%
  \titleformat{\chapter}[display]{\RaggedRight}
    {\hspace*{-\imHang}\makebox[\imFullWidth][l]{\im@display\fontsize{30}{36}\selectfont
      Chapter \thechapter\hfill\rule{25mm}{25mm}}}{6pt}
    {\im@fullhead{\sffamily\bfseries\fontsize{14}{16}\selectfont\MakeUppercase{#1}}}%
    [{\hspace*{-\imHang}\rule{\imFullWidth}{1pt}}]}{}
\im@ifmodel{lotus}{%
  \titleformat{\chapter}[display]{\RaggedRight\sffamily\bfseries\fontsize{18}{20}\selectfont}
    {Chapter \thechapter}{6pt}{\im@fullhead{#1}}}{}
\im@ifmodel{wordperfect}{%
  \titleformat{\chapter}[display]{\RaggedRight}{\titlerule[3pt]}{6pt}
    {\im@fullhead{\im@condensed\bfseries\fontsize{24}{28}\selectfont #1}}}{}
\im@ifmodel{borland}{%
  \titleformat{\chapter}[display]{\raggedleft\im@adventor}
    {\im@borlabel}
    {6pt}{\im@fullhead{\raggedleft\itshape\fontsize{24}{28}\selectfont #1}}}{}
\titlespacing*{\chapter}{0pt}{0pt}{26pt}
\im@ifmodel{borland}{\titlespacing*{\chapter}{0pt}{0pt}{12mm}}{}
%% Side head: Helvetica Condensed Bold 9/11 in the left column (Sec 4).
%% The head sits on the baseline of the first line of what follows it, which
%% may be a paragraph, a syntax line or a typed example.
\newcommand\SideHead[1]{\im@ifmodel{lotus}{\par\addvspace{6pt}%
  {\noindent\imHeadTwoFont #1\par}\nobreak\@afterheading}{\par\addvspace{6.5pt}%
  {\noindent\llap{\smash{\parbox[t]{\imSideCol}{\RaggedRight\imSideHeadFont
    \strut#1\strut}}\hspace{\imSideGap}}\par}%
  \nobreak\vspace{-\baselineskip}\vspace{-\parskip}\@afterheading}}

%% Chapter opener: always recto, chapter folio restarts, foot title set.
\let\im@origchapter\chapter
\newcommand\im@chapterhook[1]{%
  \cleardoublepage
  \gdef\im@draftfoot{}%
  \gdef\im@drafttoc{}%
  \gdef\im@idxspecial{}%
  \stepcounter{chapter}%
  \ifcsname im@draft@\im@draftkey\endcsname
    \gdef\im@draftfoot{DRAFT -- NOT FOR RELEASE}%
    \gdef\im@drafttoc{ (DRAFT)}%
    \AddToHookNext{shipout/foreground}{\put(\dimexpr65mm\relax,-\dimexpr120mm\relax){%
      \rotatebox{45}{\color{black!40}\sffamily\bfseries\fontsize{48}{48}\selectfont DRAFT}}}%
  \fi
  \addtocounter{chapter}{-1}%
  \ifim@continuous\else
    \stepcounter{chapter}\im@setfolio{\thechapter-}%
    \im@ifmodel{ashtontate}{\im@setfolio{\im@sectionprefix\thechapter-}}{}%
    \addtocounter{chapter}{-1}%
    \im@folioform
    \setcounter{page}{1}%
  \fi
  \FootTitle{#1}%
  %% the label of the chapter being opened, fixed now: \appendix may change
  %% the numbering before this chapter's last page ships.
  \begingroup\advance\c@chapter by 1
    \xdef\im@footlabel{\im@chaptername\noexpand\ \thechapter.\noexpand\ }\endgroup}
\RenewDocumentCommand\chapter{s o m}{%
  \im@closeentry
  \IfBooleanTF{#1}
    {\cleardoublepage\FootTitle{#3}\im@notab
     \im@origchapter*{#3}}
    {\im@chapterhook{#3}\im@applytab
     \im@slipglobal{heading}%
     \IfValueTF{#2}{\im@origchapter[#2\im@drafttoc]{#3}}{\im@origchapter[#3\im@drafttoc]{#3}}%
     \im@slipreset}%
  \thispagestyle{plain}%
  \IfBooleanF{#1}{\im@ifmodel{wordperfect}{%
    \thispagestyle{empty}%
    \AtPage{168.8}{15}{\colorbox{\im@dividercolor}{\parbox[c][190.6mm][c]{9mm}{%
      \centering\color{white}\rotatebox{-90}{\im@condensed\bfseries\fontsize{9}{10}\selectfont #3}}}}%
    \newpage\null\thispagestyle{empty}\newpage}{} }%
  \IfBooleanF{#1}{\im@ifmodel{ibm}{\im@ibmopener}{}}}

%% PK-07: the divider bar is a tab, so it keeps a colour -- the Office
%% Systems colour in the Reference; black in the Workbook (\DividerColor).
\newcommand\im@dividercolor{FamilyOffice}
\newcommand\DividerColor[1]{\gdef\im@dividercolor{#1}}

%% Unnumbered front-matter chapter (Preface, Contents): roman folios.
\newcounter{im@frontchapters}
\NewDocumentCommand\FrontChapter{m}{%
  \stepcounter{im@frontchapters}%
  \im@closeentry
  \cleardoublepage\FootTitle{#1}\im@notab
  \im@origchapter*{#1}\thispagestyle{plain}%
  \addcontentsline{toc}{chapter}{#1}}

%% Appendices: "Appendix A", folios "A-1".
\renewcommand\appendix{\par
  \setcounter{chapter}{0}%
  \renewcommand\thechapter{\Alph{chapter}}%
  \renewcommand\im@chaptername{Appendix}%
  \renewcommand\im@displayword{A\hfill P\hfill P\hfill E\hfill N\hfill D\hfill I\hfill X}%
  \addtocontents{toc}{\protect\renewcommand\protect\im@tocname{Appendix}}}

%% "What This Chapter Covers" (Ashton-Tate pattern) under the opener.
\NewDocumentEnvironment{ChapterCovers}{+b}
  {\im@ifmodel{ibm}{}{\par\noindent\hspace*{-\imHang}%
   \begin{minipage}[t]{\dimexpr\imTextWidth+\imHang\relax}\RaggedRight
   {\imHeadTwoFont What This Chapter Covers\par}\vspace{3pt}
   \begin{list}{\textbullet}{\leftmargin=\imHang \labelwidth=1em
      \labelsep=0.5em \itemsep=0pt \parsep=0pt \topsep=0pt}#1%
    \end{list}\end{minipage}\par\vspace{12pt}}}
  {}

%% ---- table of contents --------------------------------------------------
\RequirePackage{titletoc}
\setcounter{tocdepth}{1}
\contentsmargin{5em}
\newcommand\im@tocname{Chapter}
\im@ifmodel{operations}{\renewcommand\im@tocname{Session}}{}
\titlecontents{chapter}[0pt]
  {\addvspace{9pt}\imHeadTwoFont}
  {\im@tocname~\thecontentslabel.\enspace}
  {}
  {\hfill\contentspage}
\titlecontents{section}[1.5em]
  {\normalsize}
  {}{}
  {\titlerule*[0.75em]{.}\contentspage}
\renewcommand\tableofcontents{\im@closeentry\cleardoublepage\FootTitle{Contents}\im@notab
  \im@origchapter*{Contents}\thispagestyle{plain}%
  \begin{adjustwidth}{-\imHang}{0pt}\im@slipglobal{contents}\@starttoc{toc}\im@slipreset
  \end{adjustwidth}}
\def\contentsname{Contents}

%% ---- figures: chapter-numbered "Figure 2-1." --------------------------------
\renewcommand\thefigure{\thechapter-\arabic{figure}}
\renewcommand\thetable{\thechapter-\arabic{table}}

%% ---- index (two columns, 8.5/10; folios "Index-1", IBM) ---------------------------
\RequirePackage{multicol}
\makeindex
\renewenvironment{theindex}
  {\im@closeentry\cleardoublepage
   \ifim@continuous\else\im@setfolio{Index-}%
     \im@ifmodel{ashtontate}{\im@setfolio{X-}}{}\setcounter{page}{1}\fi
   \FootTitle{Index}\im@notab
   \im@origchapter*{Index}\thispagestyle{plain}%
   \addcontentsline{toc}{chapter}{Index}%
   \begin{adjustwidth}{-\imHang}{0pt}\im@indexpreamble
   \begin{multicols}{2}\@setfontsize\im@x{8.5}{10}%
   \setlength\parskip{0pt}\setlength\parindent{0pt}%
   \RaggedRight\im@slipat{index}%
   \let\item\im@idxitem}
  {\end{multicols}\end{adjustwidth}}
\newcommand\im@idxitem{\par\hangindent 1.5em\relax}
%% Material printed under the Index heading before the entries (F-ATE: the
%% boxed Key to Index Page Numbers, AN-24); empty unless a book sets it.
\newcommand\im@indexpreamble{}
\newcommand\IndexPreamble[1]{\gdef\im@indexpreamble{#1}}
\renewcommand\subitem{\par\hangindent 2.5em\hspace*{1em}}
\renewcommand\subsubitem{\par\hangindent 3.5em\hspace*{2em}}
\renewcommand\indexspace{\par\vspace{5.5pt plus 2pt}}
\newcommand\IndexLetter[1]{{\imSideHeadFont #1}\par\nopagebreak}
\newcommand\printindex{\InputIfFileExists{\jobname.ind}{}{%
  \ClassWarningNoLine{initech-manual}{No index file \jobname.ind yet}}}

%% ---- reference-entry continuation title (PG-10; every family) -------------
%% A reference entry that runs over a page repeats its two-line title at the
%% top of each continuation page, flush to the OUTSIDE edge, no rule (IBM
%% 3.10 Reference PDF p. 120-121).  The entry environments (initech-syntax:
%% CommandEntry, DosEntry) call \im@openentry{line 1}{line 2};
%% \im@closeentry ships the entry's last page while its title is still in
%% force, then forgets it.  Drawn at shipout, not as a fancyhdr head, which
%% would claim head height and move the text block: the two lines end 15 mm
%% from the top trim, flush with the outside edge of the full block.
\newif\ifim@inentry
\newcount\im@entrystart
\newcommand\im@entrytitleA{}
\newcommand\im@entrytitleB{}
\newcommand\imContTitleFont{\sffamily\bfseries\@setfontsize\im@x{11}{13}}
\newcommand\im@openentry[2]{\im@closeentry
  \gdef\im@entrytitleA{#1}\gdef\im@entrytitleB{#2}%
  \global\im@entrystart=\c@page \global\im@inentrytrue}
\newcommand\im@closeentry{\ifim@inentry\clearpage\global\im@inentryfalse\fi}
%% \ContinuationTitle{line 1}{line 2} inside an entry: the continuation pages
%% repeat these lines instead of the entry's own title (the paste-up of the
%% period books set them apart).
\newcommand\ContinuationTitle[2]{\gdef\im@entrytitleA{#1}\gdef\im@entrytitleB{#2}}
\AddToHook{shipout/foreground}{%
  \ifim@inentry\ifnum\c@page>\im@entrystart
    \ifodd\c@page
      \put(\dimexpr\paperwidth-\imOutside\relax,-15mm){\makebox(0,0)[rb]{%
        \im@slipat{running-head}\imContTitleFont\begin{tabular}[b]{@{}r@{}}\im@entrytitleA\\\im@entrytitleB\end{tabular}}}%
    \else
      \put(\imOutside,-15mm){\makebox(0,0)[lb]{%
        \im@slipat{running-head}\imContTitleFont\begin{tabular}[b]{@{}l@{}}\im@entrytitleA\\\im@entrytitleB\end{tabular}}}%
    \fi
  \fi\fi}

%% ---- thumb tabs (PG-30) ---------------------------------------------------
%% \ChapterTab{LABEL} BEFORE \chapter names the tab of the chapter it opens
%% (default "Chapter N" / "Appendix A"); the chapter applies it once the
%% previous chapter's pages have shipped and takes the next of the six
%% positions, counted through the whole book.  Front matter and the index
%% carry no tab.
\def\im@tablabel{}
\def\im@nexttab{}
\newcommand\ChapterTab[1]{\gdef\im@nexttab{#1}}
\newcount\im@tabseq
\newcount\im@tabpos
\newcommand\im@notab{\gdef\im@tablabel{}}
\newcommand\im@applytab{%
  %% (\im@chapterhook has left the counter at the previous chapter.)
  \ifx\im@nexttab\@empty
    \begingroup\advance\c@chapter by 1
      \xdef\im@tablabel{\im@chaptername\space\thechapter}\endgroup
  \else
    \global\let\im@tablabel\im@nexttab \gdef\im@nexttab{}\fi
  \global\im@tabpos=\im@tabseq
  \loop\ifnum\im@tabpos>5 \global\advance\im@tabpos by -6 \repeat
  \global\advance\im@tabseq by 1 }

%% ---- F-IBM furniture (PG-20, PG-28, PG-30, PG-35, AN-22) ------------------
\ifim@ibm
  %% Running text quotes long messages, paths and commands in Cursor that
  %% cannot be broken (PG-09): ragged right with a free right margin, as the
  %% IBM model sets it, instead of ragged2e's 4 em stretch.
  \emergencystretch=3em
  \setlength\RaggedRightRightskip{0pt plus 1fil}
  \contentsmargin{4.5em}
  %% PG-35: chapter lines bold with bold folio, dot leaders.
  \titlecontents{chapter}[0pt]
    {\addvspace{7pt}\sffamily\bfseries}
    {\im@tocname~\thecontentslabel.\enspace}
    {}
    {\titlerule*[0.75em]{.}\contentspage}
  %% AN-22: the chapter mini-contents ("Contents", 3.10 form) under the title.
  \titlecontents{lsection}[0pt]
    {\sffamily\bfseries\fontsize{9}{12}\selectfont}
    {}{}
    {\titlerule*[0.75em]{.}\contentspage}
  \titlecontents{lsubsection}[1.5em]
    {\fontsize{9.5}{12}\selectfont}
    {}{}
    {\titlerule*[0.75em]{.}\contentspage}
  %% PG-20: the rest of the opener is empty; PG-28 the verso behind it is
  %% "Notes:" (\cleardoublepage above); text starts on the next recto.
  \newcommand\im@ibmopener{%
    \startcontents[chapters]%
    \par\noindent\hspace*{-\imHang}%
    \begin{minipage}[t]{\imFullWidth}\RaggedRight\im@slipat{contents}%
      {\imHeadTwoFont Contents\par}\vspace{4pt}%
      \printcontents[chapters]{l}{1}{\setcounter{tocdepth}{2}}%
    \end{minipage}\par
    \cleardoublepage}
  %% PG-30: black tabs bleeding off the fore-edge of every recto in a
  %% chapter, 9 mm visible x 30 mm, label reversed reading bottom to top.
  \AddToHook{shipout/background}{%
    \ifodd\c@page\ifx\im@tablabel\@empty\else
      \put(\dimexpr\paperwidth-9mm\relax,-\dimexpr24mm+30mm*\im@tabpos+30mm\relax){%
        \textcolor{InitechBlack}{\rule{12mm}{30mm}}}%
      \put(\dimexpr\paperwidth-4.5mm\relax,-\dimexpr24mm+30mm*\im@tabpos+15mm\relax){%
        \makebox(0,0){\rotatebox{90}{\textcolor{white}{\im@condensed\bfseries
          \fontsize{7}{8}\selectfont\MakeUppercase{\im@tablabel}}}}}%
    \fi\fi}
\fi

%% ---- the Initech logo (PUB-REF-0001 D-8 revised; pubs/common) -------------
\RequirePackage{tikz}
\input{initech-logo.tex}

%% ---- VO-30: section heads print in the place `heading' ---------------------
%% titlesec sets these block heads at once, inside the command, so the place
%% holds exactly while the head is typeset (initech-syntax \Slip).
\AtBeginDocument{%
  \NewCommandCopy\im@origsection\section
  \NewCommandCopy\im@origsubsection\subsection
  \RenewDocumentCommand\section{som}{\im@slipglobal{heading}%
    \IfBooleanTF{#1}{\im@origsection*{#3}}{\IfValueTF{#2}{\im@origsection[#2]{#3}}{\im@origsection{#3}}}%
    \im@slipreset}%
  \RenewDocumentCommand\subsection{som}{\im@slipglobal{heading}%
    \IfBooleanTF{#1}{\im@origsubsection*{#3}}{\IfValueTF{#2}{\im@origsubsection[#2]{#3}}{\im@origsubsection{#3}}}%
    \im@slipreset}}

%% ---- helpers -----------------------------------------------------------
\RequirePackage{initech-syntax}
\RequirePackage{initech-screen}
\RequirePackage{initech-forms}
\RequirePackage{initech-xbase}
\endinput
