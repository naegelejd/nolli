" Joseph Naegele, 2014
"
" Adapted from go.vim:
"
" Copyright 2011 The Go Authors. All rights reserved.
" Use of this source code is governed by a BSD-style
" license that can be found in the LICENSE file.
"
" indent/nolli.vim: Vim indent file for Nolli.
"
" TODO:
" - function invocations split across lines
" - general line splits (line ends in an operator)

if exists("b:did_indent")
    finish
endif
let b:did_indent = 1

setlocal nolisp
setlocal autoindent
setlocal indentexpr=NolliIndent(v:lnum)
setlocal indentkeys+=<:>,0=},0=)

if exists("*NolliIndent")
  finish
endif

function! NolliIndent(lnum)
  let prevlnum = prevnonblank(a:lnum-1)
  if prevlnum == 0
    " top of file
    return 0
  endif

  " grab the previous and current line, stripping comments.
  let prevl = substitute(getline(prevlnum), '#.*$', '', '')
  let thisl = substitute(getline(a:lnum), '#.*$', '', '')
  let previ = indent(prevlnum)

  let ind = previ

  if prevl =~ '[({]\s*$'
    " previous line opened a block
    let ind += &sw
  endif

  if thisl =~ '^\s*[)}]'
    " this line closed a block
    let ind -= &sw
  endif

  return ind
endfunction
