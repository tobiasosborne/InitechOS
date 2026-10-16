%% initech-manual.cls -- Initech Systems Corporation house class for manuals.
%%
%% Bead initech-ipu5.5 (Initech Publications toolchain).  LuaLaTeX only
%% (decision D-11, docs/packaging/MANUALS-AND-BOXES-RESEARCH.md Sec 9).
%% The manuals are documents, not factory code: TeX is treated as an external
%% tool, like nasm (research Sec 7).  The SPEC for everything below is
%% docs/packaging/DESIGN-SYSTEM.md Sec 4 (type), Sec 5 (grid), Sec 6 (covers,
%% spines), Sec 10 (part numbers, edition notice); PM decisions 2026-10-16:
%% D-4 one 7 x 9 in trim, D-6 black-only interior (colour only on covers and
%% tabs), D-8 wordmark only, D-12 fictional address and 555-01xx numbers,
%% D-14 document gated behaviour only, D-15 honest colophon.
%%
%% ------------------------------------------------------------------------
%% CLASS DOCUMENTATION
%% ------------------------------------------------------------------------
%% Options
%%   chapterfolio   (default) chapter-page folios "2-6", front matter roman,
%%                  appendices "A-1", index "X-1" (IBM/Lotus reference style)
%%   continuous     continuous arabic folios (Borland; tutorials, Turbo Initech)
%%   binder         binder pages: inside margin 25 mm instead of 22 mm
%%   final          every \draft marker becomes a hard error (ship gate)
%%   cover          one-page cover/title insert (colour allowed, D-6)
%%   spine          binder spine insert; width from \SpineWidth (see below)
%%
%% Font substitution table (DESIGN-SYSTEM.md Sec 4 names the PostScript 35
%% faces a 1991 publications department would have used; we use the free,
%% metric-compatible TeX Gyre clones shipped with TeX Live, loaded by FILE
%% NAME through kpathsea so no system fontconfig lookup can change the
%% output -- Rule 11):
%%
%%   Role                         Period face (spec)        Used here (file)
%%   ---------------------------  ------------------------  -------------------------------
%%   Body text 10/13               New Century Schoolbook    TeX Gyre Schola (texgyreschola-*)
%%   Chapter title 24/28           Helvetica Bold            TeX Gyre Heros Bold
%%   Section heads 13/16, 10.5/13  Helvetica Bold            TeX Gyre Heros Bold
%%   Side heads, table heads 9/11  Helvetica Condensed Bold  TeX Gyre Heros Cn Bold
%%   Running foot, captions 8/10   Helvetica                 TeX Gyre Heros
%%   Typed input, screens 9/12     Courier                   TeX Gyre Cursor
%%   Key names                     Helvetica Bold sm. caps   TeX Gyre Heros Bold, caps at 80%
%%   Cover and box titles 30-40    Times Roman               TeX Gyre Termes
%%   Legal text 6.5/8              Helvetica                 TeX Gyre Heros
%%
%%   Licence: GUST Font License (all TeX Gyre faces).  The URW base35 clones
%%   (C059, Nimbus Sans, Nimbus Mono PS) are installed on the build host too
%%   and are the documented alternative; TeX Gyre is preferred because it
%%   ships inside TeX Live (one dependency, kpathsea-resolved).
%%
%% Grid (DESIGN-SYSTEM.md Sec 5, perfect-bound): trim 7 x 9 in; margins inside
%%   22, outside 16, top 19, bottom 22 mm; side-head column 36 mm + gap 6 mm
%%   + text column 98 mm = 140 mm.  The side-head column is on the LEFT of
%%   both pages (IBM/Ashton-Tate hanging side heads), so the text block sits
%%   at 64 mm from the left edge on a recto (22 + 42) and 58 mm on a verso
%%   (16 + 42); the outside/inside margins at the paper edge are exactly the
%%   specified 16/22 mm on both.  Baseline 13 pt; 40 lines + topskip.
%%   Running foot only (no running head): outside the folio, inside the
%%   chapter title, over a 0.5 pt rule spanning the full 140 mm.
%%
%% Reproducibility (Rule 11): the class suppresses every optional PDF info
%%   field (/ID, dates, producer, file name; \pdfvariable suppressoptionalinfo)
%%   and the build pins SOURCE_DATE_EPOCH; `make test-pubs-repro` proves two
%%   builds are byte-identical and `make test-pubs-repro-mutant` proves the
%%   check bites when a timestamp is injected.
%%
%% Helpers: initech-syntax.sty (notation, keys, command entries, notes,
%%   steps, DRAFT markers), initech-screen.sty (80x25 screen reproductions),
%%   initech-forms.sty (title page, edition notice, cover, spine, wordmark,
%%   Reader's Comment Form).  All sources are ASCII (Rule 12).
%% ------------------------------------------------------------------------
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{initech-manual}[1992/03/01 Initech manual class]

\RequirePackage{iftex}
\RequireLuaTeX

\newif\ifim@continuous
\newif\ifim@binder
\newif\ifim@final
\newif\ifim@cover
\newif\ifim@spine
\DeclareOption{chapterfolio}{\im@continuousfalse}
\DeclareOption{continuous}{\im@continuoustrue}
\DeclareOption{binder}{\im@bindertrue}
\DeclareOption{final}{\im@finaltrue}
\DeclareOption{cover}{\im@covertrue}
\DeclareOption{spine}{\im@spinetrue}
\DeclareOption*{\ClassError{initech-manual}{Unknown option `\CurrentOption'}{}}
\ProcessOptions\relax

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

%% ---- fonts (DESIGN-SYSTEM Sec 4; substitution table above) ---------------
\RequirePackage{fontspec}
\defaultfontfeatures{Ligatures=TeX}
\setmainfont{texgyreschola}[
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

%% Type sizes.  Body 10/13 (Sec 4).
\renewcommand\normalsize{\@setfontsize\normalsize{10}{13}%
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
\newcommand\imCaptionFont{\sffamily\@setfontsize\im@x{8}{10}}
\newcommand\imLegalFont{\sffamily\@setfontsize\im@x{6.5}{8}}
\newcommand\imTypeFont{\ttfamily\@setfontsize\im@x{9}{12}}

%% ---- page grid (DESIGN-SYSTEM Sec 5) --------------------------------------
\newlength\imSideCol   \setlength\imSideCol{36mm}
\newlength\imSideGap   \setlength\imSideGap{6mm}
\newlength\imHang      \setlength\imHang{42mm}      % side column + gap
\newlength\imFullWidth \setlength\imFullWidth{140mm}
\newlength\imInside    \setlength\imInside{22mm}
\newlength\imOutside   \setlength\imOutside{16mm}
\ifim@binder \setlength\imInside{25mm}\setlength\imFullWidth{137mm}\fi

\RequirePackage{calc}
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
    textwidth=\imFullWidth-\imHang, top=19mm,
    textheight=530pt, % 10 pt topskip + 40 x 13 pt baselines (Sec 5)
    footskip=11mm, headheight=0pt, headsep=0pt,
    inner=\imInside+\imHang, nomarginpar}
  %% Side-head column on the LEFT of both pages: on a verso the left (outside)
  %% margin is 16 mm + 42 mm.  geometry mirrors margins, so set the verso
  %% offset ourselves once geometry has done its work.
  \AtBeginDocument{%
    \setlength\evensidemargin{\imOutside+\imHang-1in}%
    \setlength\oddsidemargin{\imInside+\imHang-1in}}
\fi\fi
\setlength\topskip{10pt}
\setlength\parindent{0pt}
\setlength\parskip{6.5pt plus 1pt}
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
\newcommand\im@folioprefix{}
\newcommand\im@setfolio[1]{\xdef\im@folioprefix{#1}}
\newcommand\im@folioform{\gdef\thepage{\im@folioprefix\arabic{page}}}
\im@folioform
\newcommand\FrontMatterFolios{\gdef\thepage{\roman{page}}}
\fancypagestyle{im@text}{%
  \fancyhf{}%
  \renewcommand\headrulewidth{0pt}%
  \renewcommand\footrulewidth{0.5pt}%
  \fancyhfoffset[LO,LE]{\imHang}%
  \fancyfoot[RO,LE]{\imFootFont\thepage}%
  \fancyfoot[LO,RE]{\imFootFont\im@foottitle}}
\fancypagestyle{plain}{%
  \fancyhf{}%
  \renewcommand\headrulewidth{0pt}%
  \renewcommand\footrulewidth{0pt}%
  \fancyhfoffset[LO,LE]{\imHang}%
  \fancyfoot[RO,LE]{\imFootFont\thepage}}
\fancypagestyle{empty}{\fancyhf{}\renewcommand\headrulewidth{0pt}%
  \renewcommand\footrulewidth{0pt}}
\pagestyle{im@text}
\newcommand\FootTitle[1]{\gdef\im@foottitle{#1}}

%% Ashton-Tate pattern: the blank verso before a recto opener is a "Notes" page.
\renewcommand\cleardoublepage{\clearpage
  \if@twoside\ifodd\c@page\else
    \thispagestyle{im@text}%
    \hbox{}\vspace*{-\topskip}\vspace*{-\parskip}%
    {\noindent\hspace*{-\imHang}\imHeadOneFont Notes\par}%
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
\titleformat{\chapter}[display]
  {\im@headrag}
  {\im@chapterlabel}
  {14pt}
  {\imChapterTitleFont #1}
  [\vspace{6pt}]
\titleformat{name=\chapter,numberless}[display]
  {\im@headrag}
  {\titlerule[2pt]}
  {14pt}
  {\imChapterTitleFont #1}
  [\vspace{6pt}]
\titlespacing*{\chapter}{-\imHang}{0pt}{26pt}
\titleformat{\section}[block]{\imHeadOneFont\im@headrag}{}{0pt}{#1}
\titlespacing*{\section}{-\imHang}{19.5pt plus 4pt minus 2pt}{6.5pt}
\titleformat{\subsection}[block]{\imHeadTwoFont\im@headrag}{}{0pt}{#1}
\titlespacing*{\subsection}{0pt}{13pt plus 3pt minus 2pt}{3pt}
%% Side head: Helvetica Condensed Bold 9/11 in the left column (Sec 4).
%% The head sits on the baseline of the first line of what follows it, which
%% may be a paragraph, a syntax line or a typed example.
\newcommand\SideHead[1]{\par\addvspace{6.5pt}%
  {\noindent\llap{\smash{\parbox[t]{\imSideCol}{\RaggedRight\imSideHeadFont
    \strut#1\strut}}\hspace{\imSideGap}}\par}%
  \nobreak\vspace{-\baselineskip}\vspace{-\parskip}\@afterheading}

%% Chapter opener: always recto, chapter folio restarts, foot title set.
\let\im@origchapter\chapter
\newcommand\im@chapterhook[1]{%
  \cleardoublepage
  \ifim@continuous\else
    \stepcounter{chapter}\im@setfolio{\thechapter-}%
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
     \IfValueTF{#2}{\im@origchapter[#2]{#3}}{\im@origchapter{#3}}}%
  \thispagestyle{plain}}

%% Unnumbered front-matter chapter (Preface, Contents): roman folios.
\NewDocumentCommand\FrontChapter{m}{%
  \cleardoublepage\FootTitle{#1}%
  \im@origchapter*{#1}\thispagestyle{plain}%
  \addcontentsline{toc}{chapter}{#1}}

%% Appendices: "Appendix A", folios "A-1".
\renewcommand\appendix{\par
  \setcounter{chapter}{0}%
  \renewcommand\thechapter{\Alph{chapter}}%
  \renewcommand\im@chaptername{Appendix}%
  \addtocontents{toc}{\protect\renewcommand\protect\im@tocname{Appendix}}}

%% "What This Chapter Covers" (Ashton-Tate pattern) under the opener.
\NewDocumentEnvironment{ChapterCovers}{}
  {\par\noindent\hspace*{-\imHang}%
   \begin{minipage}[t]{\imFullWidth}\RaggedRight
   {\imHeadTwoFont What This Chapter Covers\par}\vspace{3pt}
   \begin{list}{\textbullet}{\leftmargin=\imHang \labelwidth=1em
      \labelsep=0.5em \itemsep=0pt \parsep=0pt \topsep=0pt}}
  {\end{list}\end{minipage}\par\vspace{13pt}}

%% ---- table of contents --------------------------------------------------
\RequirePackage{titletoc}
\setcounter{tocdepth}{1}
\contentsmargin{3em}
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
\renewcommand\tableofcontents{\cleardoublepage\FootTitle{Contents}%
  \im@origchapter*{Contents}\thispagestyle{plain}\@starttoc{toc}}
\def\contentsname{Contents}

%% ---- figures: chapter-numbered "Figure 2-1." --------------------------------
\renewcommand\thefigure{\thechapter-\arabic{figure}}
\renewcommand\thetable{\thechapter-\arabic{table}}

%% ---- index (two columns, 8.5/11; folios "X-1", IBM) ---------------------------
\RequirePackage{multicol}
\makeindex
\renewenvironment{theindex}
  {\cleardoublepage
   \ifim@continuous\else\im@setfolio{X-}\setcounter{page}{1}\fi
   \FootTitle{Index}%
   \im@origchapter*{Index}\thispagestyle{plain}%
   \addcontentsline{toc}{chapter}{Index}%
   \begin{multicols}{2}\@setfontsize\im@x{8.5}{11}%
   \setlength\parskip{0pt}\setlength\parindent{0pt}%
   \RaggedRight
   \let\item\im@idxitem}
  {\end{multicols}}
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

\endinput
