" This test is in a separate file, because it usually causes reports for memory
" leaks under valgrind.  That is because when fork/exec fails memory is not
" freed.  Since the process exists right away it's not a real leak.

if !has('terminal')
  finish
endif

source shared.vim

func Test_terminal_redir_fails()
  if has('unix')
    let buf = term_start('xyzabc', {'err_io': 'file', 'err_name': 'Xfile'})
    call term_wait(buf)
    call WaitFor('len(readfile("Xfile")) > 0')
    call assert_match('executing job failed', readfile('Xfile')[0])
    call WaitFor('!&modified')
    call delete('Xfile')
    bwipe
  endif
endfunc
