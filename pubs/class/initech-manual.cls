%% Initech Publications house class. Binding reference: PUB-REF-0001,
%% docs/pubs/DESIGN-REFERENCE.md, especially PG-01..35, TY-01..21, PK-02,
%% PN-01..06 and Sec 11.5. LuaLaTeX is the D-11 document exception.
%% Options: ibm (default), ashtontate, lotus, wordperfect, borland, apple,
%% operations; binder (same 22 mm inside margin); cover; spine; final.
%% The physical block is 142 x 186.3 mm, margins 22/13.8 mm, columns
%% 34 + 4 + 104 mm, fixed left on BOTH pages. Lotus uses 132 mm measure.
%% All font files are pinned in pubs/fonts.lock. PDF optional dates and ID
%% are suppressed; test-pubs-repro compares independent clean builds.
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
\DeclareOption*{\ClassError{initech-manual}{Unknown option `\CurrentOption'}{}}
\ProcessOptions\relax
\im@ifmodel{apple}{\im@continuoustrue}{}
\im@ifmodel{borland}{\im@continuoustrue}{}
\im@ifmodel{wordperfect}{\im@continuoustrue}{}

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
\fi\fi
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

%% ---- folios and running foot ----------------------------------------------
\RequirePackage{fancyhdr}
\newcommand\im@foottitle{}
\newcommand\im@draftfoot{}
\newif\ifim@entryactive
\newcount\im@entrypage
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
  \im@ifmodel{ibm}{\fancyhead[RO,LE]{\ifim@entryactive
    \ifnum\value{page}>\im@entrypage\sffamily\bfseries\im@entryname\\Command\fi\fi}}{}%
  \im@ifmodel{ibm}{}{\fancyfoot[LO,RE]{\imFootFont\im@foottitle}}%
  \im@ifmodel{operations}{\renewcommand\headrulewidth{0.5pt}%
    \renewcommand\footrulewidth{0.5pt}%
    \fancyfoot[RE]{\imFootFont\im@product\ \im@doctitle}%
    \fancyfoot[LO]{\imFootFont Session \thechapter.\ \im@foottitle}}{}%
  \im@ifmodel{ashtontate}{\renewcommand\headrulewidth{0.5pt}%
    \fancyhead[RE]{\imFootFont\bfseries CHAPTER \thechapter}%
    \fancyhead[LO]{\imFootFont\bfseries\MakeUppercase{\im@foottitle}}%
    \fancyhead[LE,RO]{\ifim@referencepages
      \sffamily\bfseries\fontsize{14}{16}\selectfont\MakeUppercase{\rightmark}\fi}%
    \fancyfoot[LO,RE]{\imFootFont\bfseries\im@product\ \im@doctitle}}{}%
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
\newcommand\FootTitle[1]{\gdef\im@foottitle{#1}}
\renewcommand\sectionmark[1]{\markright{#1}}

%% Ashton-Tate pattern: the blank verso before a recto opener is a "Notes" page.
\renewcommand\cleardoublepage{\clearpage
  \if@twoside\ifodd\c@page\else
    \im@ifmodel{borland}{\thispagestyle{im@text}}{\thispagestyle{empty}}%
    \hbox{}%
    \im@ifmodel{ibm}{\thispagestyle{im@text}%
      {\noindent\hspace*{-\imHang}\imHeadOneFont Notes:\par}}{}%
    \newpage
  \fi\fi}

%% ---- headings (Sec 4, Sec 5 chapter opener) -------------------------------
\RequirePackage[explicit]{titlesec}
\RequirePackage{minitoc}
\setcounter{minitocdepth}{2}
\renewcommand\mtctitle{Contents}
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
  \titleformat{\section}[block]{\imHeadOneFont}{}{0pt}{\im@fullhead{#1}}%
  \titlespacing*{\section}{0pt}{18pt}{6pt}}{}
\im@ifmodel{operations}{%
  \titleformat{\chapter}[display]{\RaggedRight}{\hspace*{-\imHang}\rule{40mm}{2pt}}{6pt}
    {\im@fullhead{\sffamily\bfseries\fontsize{16}{18}\selectfont Session \thechapter.\ #1}}}{}
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
  \stepcounter{chapter}%
  \ifcsname im@draft@\thechapter\endcsname
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
  \FootTitle{#1}}
\RenewDocumentCommand\chapter{s o m}{%
  \IfBooleanTF{#1}
    {\cleardoublepage\FootTitle{#3}%
     \im@origchapter*{#3}}
    {\im@chapterhook{#3}%
     \IfValueTF{#2}{\im@origchapter[#2\im@drafttoc]{#3}}{\im@origchapter[#3\im@drafttoc]{#3}}}%
  \thispagestyle{plain}%
  \IfBooleanF{#1}{\im@ifmodel{wordperfect}{%
    \thispagestyle{empty}%
    \AtPage{168.8}{15}{\colorbox{FamilyOffice}{\parbox[c][190.6mm][c]{9mm}{%
      \centering\color{white}\rotatebox{-90}{\im@condensed\bfseries\fontsize{9}{10}\selectfont #3}}}}%
    \newpage\null\thispagestyle{empty}\newpage}{} }%
  \IfBooleanF{#1}{\im@ifmodel{ibm}{%
    \noindent\hspace*{-\imHang}\begin{minipage}{\imFullWidth}\minitoc\end{minipage}%
    \AtPage{168.8}{\fpeval{19+30*(\value{chapter}-1-6*floor((\value{chapter}-1)/6))}}{%
      \colorbox{black}{\parbox[c][28mm][c]{9mm}{\centering\color{white}%
        \rotatebox{90}{\im@condensed\bfseries\fontsize{7}{8}\selectfont\im@chaptername\ \thechapter}}}}%
    \newpage\thispagestyle{plain}\noindent\hspace*{-\imHang}%
    {\imHeadOneFont Notes:\par}\newpage}{} }}

%% Unnumbered front-matter chapter (Preface, Contents): roman folios.
\newcounter{im@frontchapters}
\NewDocumentCommand\FrontChapter{m}{%
  \stepcounter{im@frontchapters}%
  \cleardoublepage\FootTitle{#1}%
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
\titlecontents{chapter}[0pt]
  {\addvspace{9pt}\imHeadTwoFont}
  {\im@tocname~\thecontentslabel.\enspace}
  {}
  {\hfill\contentspage}
\titlecontents{section}[1.5em]
  {\normalsize}
  {}{}
  {\titlerule*[0.75em]{.}\contentspage}
\renewcommand\tableofcontents{\dominitoc\cleardoublepage\FootTitle{Contents}%
  \im@origchapter*{Contents}\thispagestyle{plain}%
  \begin{adjustwidth}{-\imHang}{0pt}\@starttoc{toc}\end{adjustwidth}%
  \setcounter{mtc}{\value{im@frontchapters}}}
\def\contentsname{Contents}

%% ---- figures: chapter-numbered "Figure 2-1." --------------------------------
\renewcommand\thefigure{\thechapter-\arabic{figure}}
\renewcommand\thetable{\thechapter-\arabic{table}}

%% ---- index (two columns, 8.5/10; folios "Index-1", IBM) ---------------------------
\RequirePackage{multicol}
\makeindex
\renewenvironment{theindex}
  {\cleardoublepage
   \ifim@continuous\else\im@setfolio{Index-}%
     \im@ifmodel{ashtontate}{\im@setfolio{X-}}{}\setcounter{page}{1}\fi
   \FootTitle{Index}%
   \im@origchapter*{Index}\thispagestyle{plain}%
   \addcontentsline{toc}{chapter}{Index}%
   \begin{adjustwidth}{-\imHang}{0pt}\begin{multicols}{2}\@setfontsize\im@x{8.5}{10}%
   \setlength\parskip{0pt}\setlength\parindent{0pt}%
   \RaggedRight
   \let\item\im@idxitem}
  {\end{multicols}\end{adjustwidth}}
\newcommand\im@idxitem{\par\hangindent 1.5em\relax}
\renewcommand\subitem{\par\hangindent 2.5em\hspace*{1em}}
\renewcommand\subsubitem{\par\hangindent 3.5em\hspace*{2em}}
\renewcommand\indexspace{\par\vspace{5.5pt plus 2pt}}
\newcommand\IndexLetter[1]{{\imSideHeadFont #1}\par\nopagebreak}
\newcommand\printindex{\InputIfFileExists{\jobname.ind}{}{%
  \ClassWarningNoLine{initech-manual}{No index file \jobname.ind yet}}}

%% ---- helpers -----------------------------------------------------------
\RequirePackage{initech-syntax}
\RequirePackage{initech-screen}
\RequirePackage{initech-forms}
%% Printer's signatures: preserve the last interior page for the colophon.
\newcount\im@sheetpages
\AddToHook{shipout/after}{\global\advance\im@sheetpages by 1}
\ExplSyntaxOn
\newcommand\PadForColophon{\clearpage
  \prg_replicate:nn {\int_mod:nn {7-\int_mod:nn {\im@sheetpages}{4}}{4}}
    {\null\thispagestyle{empty}\newpage}}
\ExplSyntaxOff

\endinput
