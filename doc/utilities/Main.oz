%%%
%%% Author:
%%%   Leif Kornstaedt <kornstae@ps.uni-sb.de>
%%%
%%% Contributor:
%%%   Denys Duchier <duchier@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Leif Kornstaedt, 1998
%%%   Denys Duchier, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation of Oz 3:
%%%    http://www.mozart-oz.org
%%%
%%% See the file "LICENSE" or
%%%    http://www.mozart-oz.org/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor
import
   Application(getCmdArgs exit)
   Property(get put)
   System(printError showInfo)
   OzDocToHTML(translate)
   OS(getEnv putEnv)
   URL
   Chunk(getChunk)
   %% for html-global-index
   Indexer(makeIndex)
   Gdbm at 'x-oz://contrib/gdbm'
   File(write)
   HTML(seq: SEQ pcdata: PCDATA toVirtualString)
prepare
   Spec = record('in'(single char: &i type: string optional: false)
                 'type'(single char: &t type: string optional: false)
                 'html'(alias: 'type'#"html-stylesheets")
                 'out'(single char: &o type: string optional: false)
                 'autoindex'(rightmost type: bool default: false)
                 %% HTML options
                 'top'(single type: string default: unit)
                 'stylesheet'(single type: string default: unit)
                 'latextogif'(rightmost type: bool default: true)
                 'latexdb'(single type: string default: unit)
                 'split'(rightmost type: bool default: true)
                 'abstract'(rightmost type: bool default: false)
                 'xrefdb'(single type: string default: unit)
                 'xrefdir'(single type: string default: unit)
                 'xreftree'(single type: string default: '../')
                 'indexdb'(single type: string default: unit)
                 'keeppictures'(rightmost type: bool default: false)
                 %% Path names
                 'ozdoc-home'(single type: string default: unit)
                 'author-path'(single type: string default: unit)
                 'bib-path'(single type: string default: unit)
                 'bst-path'(single type: string default: unit)
                 'elisp-path'(single type: string default: unit)
                 'sbin-path'(single type: string default: unit)
                 'catalog'(single type: string default: unit)
                 'include'(multiple type: string default: nil)
                )
define
   try
      Args = {Application.getCmdArgs Spec}
   in
      %% Process path name options and store results in ozdoc.* properties
      local
         %% Determine the directory in which document source files are located:
         SRC_DIR =
         local
            X    = Args.'in'
            Url  = {URL.make X}
            Path = {CondSelect Url path unit}
         in
            if Path==unit orelse Path.1==nil then
               '.'
            else
               Lab = {Label Path}
               L1  = Path.1
               N   = {Length L1}
               L2  = {List.take L1 N-1}
            in
               case L2 of nil then
                  {URL.toString {AdjoinAt Url path Lab(["."#false])}}
               elsecase {Reverse L2} of (C#_)|L then
                  {URL.toString {AdjoinAt Url path
                                 Lab({Reverse (C#false)|L})}}
               end
            end
         end
         {Property.put 'ozdoc.src.dir' SRC_DIR}
         OZDOC_HOME =
         case Args.'ozdoc-home' of unit then
            case {OS.getEnv 'OZDOC_HOME'} of false then
               {Property.get 'oz.home'}#'/share/doc'
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.home' OZDOC_HOME}
         AUTHOR_PATH =
         case Args.'author-path' of unit then
            case {OS.getEnv 'OZDOC_AUTHOR_PATH'} of false then
               SRC_DIR#':'#OZDOC_HOME
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.author.path' AUTHOR_PATH}
         BIB_PATH =
         case Args.'bib-path' of unit then
            case {OS.getEnv 'OZDOC_BIB_PATH'} of false then
               SRC_DIR#':'#OZDOC_HOME
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.bib.path' BIB_PATH}
         BST_PATH =
         case Args.'bst-path' of unit then
            case {OS.getEnv 'OZDOC_BST_PATH'} of false then
               BIB_PATH
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.bst.path' BST_PATH}
         ELISP_PATH =
         case Args.'elisp-path' of unit then
            case {OS.getEnv 'OZDOC_ELISP_PATH'} of false then
               SRC_DIR#':'#{Property.get 'oz.home'}#'/share/elisp'
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.elisp.path' ELISP_PATH}
         SBIN_PATH =
         case Args.'sbin-path' of unit then
            case {OS.getEnv 'OZDOC_SBIN_PATH'} of false then
               OZDOC_HOME
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.sbin.path' SBIN_PATH}
         CSS =
         case Args.'stylesheet' of unit then
            case {OS.getEnv 'OZDOC_STYLESHEET'} of false then
               'http://www.mozart-oz.org/home/doc/ozdoc.css'
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.stylesheet' CSS}
         CATALOG =
         case Args.'catalog' of unit then
            case {OS.getEnv 'OZDOC_CATALOG'} of false then
               OZDOC_HOME#'/catalog'
            elseof X then X end
         elseof X then X end
         {Property.put 'ozdoc.catalog' CATALOG}
         INCLUDE =
         case Args.'include' of unit then
            case {OS.getEnv 'OZDOC_INCLUDE'} of false then
               nil
            elseof X then {String.tokens X &:} end
         elseof X then X end
         {Property.put 'ozdoc.include' INCLUDE}
         TEXINPUTS =
         '.:'#
         case {OS.getEnv 'OZDOC_TEXINPUTS'} of false then
            SRC_DIR#':'
         elseof X then X end
         #':'#
         case {OS.getEnv 'TEXINPUTS'} of false then nil
         elseof X then X end
      in
         {OS.putEnv 'PATH' SBIN_PATH#':'#{OS.getEnv 'PATH'}}
         {OS.putEnv 'OZDOC_ELISP_PATH' ELISP_PATH}
         {OS.putEnv 'EMACS_UNIBYTE' 'yes'}
         {OS.putEnv 'TEXINPUTS' TEXINPUTS}
      end
      %% The actual translation
      case Args.1 of _|_ then
         {Raise usage('extra command line arguments')}
      elsecase Args.'type' of "html-color" then
         {OzDocToHTML.translate color Args}
      elseof "html-mono" then
         {OzDocToHTML.translate mono Args}
      elseof "html-stylesheets" then
         {OzDocToHTML.translate stylesheets Args}
      elseof "html-global-index" then DB Xs HTML1 HTML2 in
         DB = {Gdbm.new read(Args.'in')}
         Xs = {Gdbm.entries DB}
         HTML1 = {Indexer.makeIndex
                  {FoldR Xs
                   fun {$ Prefix#(DocumentTitle#Entries) Rest}
                      {FoldR Entries
                       fun {$ Ands#(RURL#SectionTitle)#_ Rest}
                          Ands#a(href: Prefix#'/'#RURL
                                 SEQ([DocumentTitle PCDATA(', ')
                                      SectionTitle]))|Rest
                       end Rest}
                   end nil}}
         {Gdbm.close DB}
         HTML2 = html(head(title(PCDATA('Global Index'))
                           link(rel: stylesheet
                                type: 'text/css'
                                href: {Property.get 'ozdoc.stylesheet'}))
                      'body'(h1(PCDATA('Global Index')) HTML1 hr()
                             address(span('class':[version]
                                          PCDATA('Version '#
                                                 {Property.get 'oz.version'}#
                                                 ' ('#{Property.get 'oz.date'}#
                                                 ')')))))
         {File.write {HTML.toVirtualString HTML2}#'\n' Args.'out'}
      elseof "chunk" then
         {System.showInfo
          {Chunk.getChunk Args.'in' Args.'out'}}
      else
         {Raise usage('illegal output type specified')}
      end
      {Application.exit 0}
   catch E then
      case E of error(ap(usage M) ...) then
         {System.printError
          'Command line option error: '#M#'\n'#
          'Usage: '#{Property.get 'application.url'}#' [options]\n'#
          '--in=<File>         The input SGML file.\n'#
          '--type=<Type>       What output to generate\n'#
          '                    (supported: '#
          'html-color html-mono html-stylesheets\n'#
          '                    html-global-index chunk).\n'#
          '--out=<Dir>         The output directory.\n'#
          '--(no)autoindex     Automatically generate index entries.\n'#
          '--include=A1,...,An Assume `<!ENTITY & Ai "INCLUDE">\'.\n'#
          '\n'#
          'HTML options\n'#
          '--top=<RURL>        What to link `Top\' to.\n'#
          '--stylesheet=<RURL> What style sheet to use for generated pages.\n'#
          '--(no)latextogif    Generate GIF files from LaTeX code.\n'#
          '--latexdb=<File>    Reuse GIFs generated from LaTeX code.\n'#
          '--(no)split         Split the document into several nodes.\n'#
          '--(no)abstract      Generate an abstract.html auxiliary file.\n'#
          '--keeppictures      Do no recreate GIF from PS if already there.\n'#
          '\n'#
          'Inter-Document Cross-Referencing\n'#
          '--xrefdb=<File>     Where to look up resp. store references.\n'#
          '--xrefdir=<RelURL>  Where this document goes relative to the\n'#
          '                    whole documentation installation directory.\n'#
          '--xreftree=<RelURL> How to get to whole doc installation from\n'#
          '                    the directory where this document goes.\n'#
          '--indexdb=<File>    Where to look up resp. store index entries.\n'#
          '\n'#
          'Parametrization\n'#
          '--ozdoc-home=<DIR>  ozdoc installation directory.\n'#
          '--author-path=<Search Path>\n'#
          '--bib-path=<Search Path>\n'#
          '--bst-path=<Search Path>\n'#
          '--sbin-path=<Search Path>\n'#
          '                    Where to look for author databases,\n'#
          '                    bib files, bst files, and ozdoc scripts.\n'#
          '--catalog=<File>    Specify the catalog file to use for parsing.\n'}
         {Application.exit 2}
      elseof error then
         {Application.exit 1}
      else
         {Raise E}
      end
   end
end
