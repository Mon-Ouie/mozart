
OZ_BI_proto(fd_init);
OZ_BI_proto(fd_add);
OZ_BI_proto(fd_twice);

extern "C"
{

  OZ_C_proc_interface *oz_init_module(void)
  {
    static OZ_C_proc_interface i_table[] = {
      {"init", 0, 0, fd_init},
      {"add", 3, 0, fd_add},
      {"twice", 2, 0, fd_twice},
      {0,0,0,0}
    };

    printf("addition propagator loaded\n");
    return i_table;
  }
}
