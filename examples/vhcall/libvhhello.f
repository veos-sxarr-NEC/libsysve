      function hellofunc(name, len)
      implicit none
      integer(8) hellofunc
      integer len
      character(len=len), intent(in) :: name
      write(*,*) '[VH libvhhello_f.so:hellofunc] 1starg:', name
      write(*,*) '[VH libvhhello_f.so:hellofunc] 2ndarg:', len
      hellofunc = 999
      return
      end function

      subroutine hellosubr(a, len)
      implicit none
      double precision a(len,len)
      integer len, i, j
      do i=1,len; do j=1,len;
        write(*,*) '[VH] a(',i,',',j,')=',a(i,j)
      end do; end do
      return
      end subroutine

