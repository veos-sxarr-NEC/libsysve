integer function hellofunc(name, len)
  implicit none
  integer, intent(in) :: len
  character(len=len), intent(in) :: name
  write(*,*) '[VH libvhhello_f.so:hellofunc] 1starg:', name
  write(*,*) '[VH libvhhello_f.so:hellofunc] 2ndarg:', len
  hellofunc = 0
  return
end function

subroutine hellosubr(name, len)
  implicit none
  integer, intent(in) :: len
  character(len=len), intent(inout) :: name
  write(*,*) '[VH libvhhello_f.so:hellosubr] 1starg:', name
  write(*,*) '[VH libvhhello_f.so:hellosubr] 2ndarg:', len
  name = 'Hello VE, This is VH Fortran library'
  return
end subroutine
