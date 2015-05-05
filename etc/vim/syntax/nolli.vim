" Vim syntax file
" Language:     Nolli
" Maintainer:   Joe Naegele <joseph.naegele@gmail.com>
" Last Change:  2013-09-21

" Quit when a (custom) syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn case match

syn keyword     nolliDirective          package from import
syn keyword     nolliDeclKind           var const
syn keyword     nolliDef                alias class interface

hi def link     nolliDirective          Statement
hi def link     nolliDeclKind           Function
hi def link     nolliDef                Macro

" Treat func specially: it's a declaration at the start of a line, but a type
" elsewhere. Order matters here.
syn match       nolliType               /\<func\>/
syn match       nolliFunction           /\(^func\|\ \+func\)\>/

hi def link     nolliFunction           Keyword

" Keywords within functions
syn keyword     nolliStatement          defer return break continue
syn keyword     nolliConditional        if else
syn keyword     nolliLoop               for while in

hi def link     nolliStatement          Statement
hi def link     nolliConditional        Conditional
hi def link     nolliLoop               Repeat

" Predefined types
syn keyword     nolliType               bool str error
syn keyword     nolliInts               char int
syn keyword     nolliFloats             real cplx

hi def link     nolliType               Type
hi def link     nolliInts               Type
hi def link     nolliFloats             Type

" Predefined functions and values
syn keyword     nolliBuiltins          print
syn keyword     nolliConstants         true false nil

hi def link     nolliBuiltins          Directory
hi def link     nolliConstants         Constant

" Comments; their contents
syn keyword     nolliTodo              contained TODO FIXME XXX BUG
syn cluster     nolliCommentGroup      contains=nolliTodo
" syn region      nolliComment           start="/\*" end="\*/" contains=@nolliCommentGroup,@Spell
syn region      nolliComment           start="#" end="$" contains=@nolliCommentGroup,@Spell

hi def link     nolliComment           Comment
hi def link     nolliTodo              Todo

" nolli escapes
syn match       nolliEscapeOctal       display contained "\\[0-7]\{3}"
syn match       nolliEscapeC           display contained +\\[abfnrtv\\'"]+
syn match       nolliEscapeX           display contained "\\x\x\{2}"
syn match       nolliEscapeU           display contained "\\u\x\{4}"
syn match       nolliEscapeBigU        display contained "\\U\x\{8}"
syn match       nolliEscapeError       display contained +\\[^0-7xuUabfnrtv\\'"]+

hi def link     nolliEscapeOctal       nolliSpecialString
hi def link     nolliEscapeC           nolliSpecialString
hi def link     nolliEscapeX           nolliSpecialString
hi def link     nolliEscapeU           nolliSpecialString
hi def link     nolliEscapeBigU        nolliSpecialString
hi def link     nolliSpecialString     Special
hi def link     nolliEscapeError       Error

" Strings and their contents
syn cluster     nolliStringGroup       contains=nolliEscapeOctal,nolliEscapeC,nolliEscapeX,nolliEscapeU,nolliEscapeBigU,nolliEscapeError
syn region      nolliString            start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=@nolliStringGroup
syn region      nolliRawString         start=+`+ end=+`+

hi def link     nolliString            String
hi def link     nolliRawString         String

" Characters; their contents
syn cluster     nolliCharacterGroup    contains=nolliEscapeOctal,nolliEscapeC,nolliEscapeX,nolliEscapeU,nolliEscapeBigU
syn region      nolliCharacter         start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=@nolliCharacterGroup

hi def link     nolliCharacter         Character

" Regions
syn region      nolliBlock             start="{" end="}" transparent fold
syn region      nolliParen             start='(' end=')' transparent

" Integers
syn match       nolliDecimalInt        "\<\d\+\([Ee]\d\+\)\?\>"
syn match       nolliHexadecimalInt    "\<0x\x\+\>"
syn match       nolliOctalInt          "\<0\o\+\>"
syn match       nolliOctalError        "\<0\o*[89]\d*\>"

hi def link     nolliDecimalInt        Integer
hi def link     nolliHexadecimalInt    Integer
hi def link     nolliOctalInt          Integer
hi def link     Integer             Number

" Floating point
syn match       nolliFloat             "\<\d\+\.\d*\([Ee][-+]\d\+\)\?\>"
syn match       nolliFloat             "\<\.\d\+\([Ee][-+]\d\+\)\?\>"
syn match       nolliFloat             "\<\d\+[Ee][-+]\d\+\>"

hi def link     nolliFloat             Float

" Imaginary literals
syn match       nolliImaginary         "\<\d\+i\>"
syn match       nolliImaginary         "\<\d\+\.\d*\([Ee][-+]\d\+\)\?i\>"
syn match       nolliImaginary         "\<\.\d\+\([Ee][-+]\d\+\)\?i\>"
syn match       nolliImaginary         "\<\d\+[Ee][-+]\d\+i\>"

hi def link     nolliImaginary         Number

" Search backwards for a global declaration to start processing the syntax.
"syn sync match nolliSync grouphere NONE /^\(const\|var\|type\|func\)\>/

" There's a bug in the implementation of grouphere. For now, use the
" following as a more expensive/less precise workaround.
syn sync minlines=500

let b:current_syntax = "nolli"
" vim:set sw=4 sts=4 et:
