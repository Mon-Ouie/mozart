#!/usr/bin/perl -w
#
# Authors:
#   Andreas Simon (2000)
#
# Copyright:
#   Andreas Simon (2000)
#
# Last change:
#   $Date$ by $Author$
#   $Revision$
#
# This file is part of Mozart, an implementation
# of Oz 3:
#   http://www.mozart-oz.org
#
# See the file "LICENSE" or
#   http://www.mozart-oz.org/LICENSE.html
# for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL
# WARRANTIES.
#

use Getopt::Long;
use POSIX qw(strftime);

$C_FUN_PREFIX = "oz"; # Prefix for OZ_BI_define'd functions

{
    my $time   = strftime "%a %b %e %H:%M:%S UTC %Y", gmtime;
    $C_WARNING = <<EOF;
/*
   THIS FILE IS AUTOMATICALLY GENERATED. PLEASE DO NOT EDIT!

                   $time
 */

EOF
}

%argument_translation =
  (
   '...'                      => 'GOZ_DECLARE_VARARG',
   'char'                     => 'OZ_declareInt',
   'unsigned char'            => 'OZ_declareInt',
   'int'                      => 'OZ_declareInt',
   'unsigned int'             => 'OZ_declareInt',
   'double'                   => 'OZ_declareFloat',
   'float'                    => 'OZ_declareFloat',

   'gboolean'                 => 'GOZ_DECLARE_GBOOLEAN',

   'gpointer'                 => 'GOZ_DECLARE_GPOINTER',

   'gchar'                    => 'GOZ_DECLARE_GCHAR',
   'guchar'                   => 'GOZ_DECLARE_GUCHAR',
   'gchar*'                   => 'GOZ_DECLARE_VIRTUAL_STRING',
   'const gchar*'             => 'GOZ_DECLARE_VIRTUAL_STRING',

   'gint'                     => 'GOZ_DECLARE_GINT',
   'guint'                    => 'GOZ_DECLARE_GUINT',
   'gshort'                   => 'GOZ_DECLARE_GSHORT',
   'gushort'                  => 'GOZ_DECLARE_GUSHORT',
   'glong'                    => 'GOZ_DECLARE_GLONG',
   'gulong'                   => 'GOZ_DECLARE_GULONG',

   'gint8'                    => 'GOZ_DECLARE_GINT8',
   'guint8'                   => 'GOZ_DECLARE_GUINT8',
   'gint16'                   => 'GOZ_DECLARE_GINT16',
   'guint16'                  => 'GOZ_DECLARE_GUINT16',
   'gint32'                   => 'GOZ_DECLARE_GINT32',
   'guint32'                  => 'GOZ_DECLARE_GUINT32',
   'gint64'                   => 'GOZ_DECLARE_GINT64',
   'guint64'                  => 'GOZ_DECLARE_GUINT64',

   'gfloat'                   => 'GOZ_DECLARE_GFLOAT',
   'gdouble'                  => 'GOZ_DECLARE_GDOUBLE',

   'GtkArg'                   => 'GOZ_DECLARE_GTKARG'
  );

%return_value_translation =
  (
   'int'                      => 'OZ_int',
   'float'                    => 'OZ_float',
   'double'                   => 'OZ_float',

   'gpointer'                 => 'OZ_makeForeignPointer',

   'gboolean'                 => 'GOZ_BOOL',

   'gchar'                    => 'OZ_int',
   'guchar'                   => 'OZ_int',

   'gchar'                    => 'GOZ_BYTE_STRING',
   'gchar*'                   => 'GOZ_BYTE_STRING',

   'gint'                     => 'OZ_int',
   'guint'                    => 'OZ_int',
   'guint32'                  => 'OZ_int',

   'glong'                    => 'OZ_int',
   'gulong'                   => 'OZ_int'
  );

###############################################################################
#
# c_name_to_oz_feature
#
# Transforms a C name as c_name_to_oz_variable does, but the first character
# is forced to be lower case.
#
###############################################################################
sub c_name_to_oz_feature($) {
    my ($name) = @_;
    $name = c_name_to_oz_variable($name);
    $name = lcfirst($name);
    return $name;
}

###############################################################################
#
# c_name_to_oz_variable
#
# Transforms a C name to an Oz variable name, while fullfill Oz naming
# conventions. 'ozgtk_box_set_child_packing' gets 'BoxSetChildPacking'
# Is is asumed that all C names gbegin with 'ozgtk_'.
#
###############################################################################
sub c_name_to_oz_variable($) {
    my ($name) = @_;
    my @substrings;
    my $string;

    $name =~ s/^oz(gtk|gdk|gnome)_//s;
    @substrings = split /_/, $name;
    foreach $string (@substrings) {
        $string = ucfirst $string;
    }
    $name = join '', @substrings;

    return $name;
}

###############################################################################
#
# get_interface_data
#
# Parses C files for OZ_BI_define'd functions and generates an array containing
# the interface data as described below.
# For each interface procedure found the array contains an reference to another
# array which contains all interface information for this procedure.
# The first field of the array is the C name of the procedure.
# The second field is the Oz name of the procedure.
# The third field is the in arity of the procedure.
# The forth field is the out arity.
#
###############################################################################

sub get_interface_data(@) {
    my @c_files = @_;
    my $file;
    my @interface_data;

    $/ = "\n"; # parse line for line
    my $comment = 0;

    foreach $file (@c_files) {
        open(INPUT, $file) || warn "Could not open file '$file'. $!\n";
        while (<INPUT>) {
            if(m/\/\*/ && m/\*\//) { next };
            $comment++ if m/\s*\/\*/;        # comment start
            $comment-- if m/\*\/\s*/;        # comment end
            next unless $comment == 0;       # ignore C comment lines
            if(m/OZ_BI_define\s*\((\w+)\,\s*(\d+)\s*\,\s*(\d+)\s*\)/) {
                my $c_name = $1;
                my $oz_name = c_name_to_oz_feature($c_name);
                my $in_arity = $2;
                my $out_arity = $3;
                my @list = ($c_name, $oz_name, $in_arity, $out_arity);
                @interface_data = (@interface_data, \@list);
            }
        }
        close INPUT;
    }
    return @interface_data;
}

# Print a warning comment and include needed headers
sub gen_prelude {
    print $C_WARNING;

    # include headers
    print "#include <mozart.h>\n";
    print "#include <goz_support.h>\n";
    foreach my $header (@includes) {
        print "#include <$header>\n";
    }
    print "\n";
}

###############################################################################
#
# gen_oz_interface
#
# searches C files for OZ_BI_defined functions and generates prototypes and the
# Oz/C procedure interface and writes them in a file
# input: an array of file names
#
###############################################################################
sub gen_oz_interface(@) {
    my @interfaces = get_interface_data @_;
    my $interface;

    gen_prelude();

    # generate prototypes for the interface functions
    # This is needed because the functions are defined in an other file
    foreach $interface (@interfaces) {
        my $c_name = $$interface[0];
        print "OZ_BI_proto($c_name);\n";
    }

    # oz_init_module
    print <<EOF;

OZ_C_proc_interface *
oz_init_module()
{
    static OZ_C_proc_interface interface[] = {
EOF

    # generate interface definitions
    foreach $interface (@interfaces) {
        my $c_name    = $$interface[0];
        my $oz_name   = $$interface[1];
        my $in_arity  = $$interface[2];
        my $out_arity = $$interface[3];
        print "        {\"$oz_name\", $in_arity, $out_arity, $c_name},\n";
    }

    print <<EOF;
        {0, 0, 0, 0}
    };
    gtk_init (0, 0);
    return interface;
}
EOF

}

# removes tag before type string
# Tags are:
#   !        Object with corresponding Oz object
#   ^        Same as ! but also allows unit to stand for NULL
#   %        Enumeration type
#   +        Out value (eg. gchar** in gtk_label_get)
#   =        In/Out value
sub clean_type {
  my ($type_str) = @_;
  $type_str =~ s/^[\%\!\+\=\^]+//s ;
  $type_str =~ s/const //sg;
  return $type_str;
}

# check whether an argument is a return value or not
sub is_return_value {
  my ($arg) = @_;
  return $arg =~ m/^\+/s;
}

# check whether an argument is an enumeration type or not
sub is_enumeration {
    my ($arg) = @_;
    return $arg =~ m/^\%/s;
}

# check whether an argument is an array type or not
sub is_array {
    my ($arg) = @_;
    return $arg =~ m/\[\w*\]/s;
}

# get the length of an array
sub array_length {
    my ($arg) = @_;
    error("array_length called for the non array type: $arg\n") unless is_array($arg);
    $arg =~ m/\[(\d*)\]/s;
    error("array_length not available in array: $arg\n") unless $1;
    return $1;
}

# converts C values to Oz terms
sub c_value2oz_term {
  my ($arg, $type) = @_;
  my $ctype = clean_type($type);

  if ($return_value_translation{$ctype}) {
      return "$return_value_translation{$ctype} ($arg)";
  } elsif (is_array($type)) {
      return '/* TODO: array return value not supported */';
  } elsif (is_enumeration($type)) {
      return "OZ_int ($arg)";
  } else {
      return "OZ_makeForeignPointer ($arg)";
  }
}

sub write_oz_bi_definition {
  my ($meths, $meth) = @_;
  my $in    = $$meths{$meth}{in};  # list of arguments
  my $out   = $$meths{$meth}{out}; # return value

  #
  # compute arities
  #
  my $arity_in          = 0;
  my $arity_out         = 0;
  my $arity_special_out = 0; # This is the out arity without the normal C return value

  my $out_flag = 0; # for testing if an IN value follows an OUT value
  foreach my $arg (@$in) {
    if (is_return_value($arg)) {
        $arity_special_out += 1;
        $out_flag = 1 unless $out_flag;
    } else {
        error("error while processing $meth:\n"
            . "IN argument following OUT argument. The BI API does not allow this.\n")
                if $out_flag;
        $arity_in += 1;
    }
  }
  $arity_out = $arity_special_out;
  $arity_out += 1 if $out;

  print "OZ_BI_define ($C_FUN_PREFIX$meth, $arity_in, $arity_out) {\n";

  #
  # function arguments
  #
  print "\t" . clean_type($out) . " ret;\n" if $out; # Standard C return value

  my $i = 0;
  foreach my $arg (@$in) {
      print "\t";
      if (is_return_value($arg)) {
          # We have to declare a variable for the C return value
          my $real_type = $arg;
          $real_type =~ s/\*$//s; # drop last asterisk
          if (is_array($real_type)) { # array types
              $real_type =~ s/(\[\w*\])/ arg$i$1;/ ;
              print clean_type($real_type) . "\n";
          } else { #pointers
              print clean_type($real_type) . " arg$i;\n";
          }
      } else {
          #
          # First lookup the translation table
          #
          if ($argument_translation{clean_type($arg)}) {
              print $argument_translation{clean_type($arg)};
              print " ($i, arg$i);\n";
          #
          # array types
          #
          } elsif (is_array($arg)) {
              my $type = clean_type($arg);
              $type =~ s/\[\d*\]/\*/g;
              print "/* Array type not supported yet */\n";
              print "\tOZ_declareForeignType ($i, arg$i, $type);\n";
          #
          # enumerations
          #
          } elsif (is_enumeration($arg)) {
              print "OZ_declareInt ($i, arg$i\_);\n";
              print "\t" . clean_type($arg) . " arg$i = (" . clean_type($arg) . ") arg$i\_;\n";
          #
          # everything else is a foreign type
          #
          } else {
              print "OZ_declareForeignType ($i, arg$i, " . clean_type($arg) . ");\n";
          }
      }
      $i++;
  }

  #
  # function invocation
  #
  print "\t";
  print 'ret = ' if $out;
  print "$meth (";
  for (my $i = 0; $i < ($arity_in + $arity_special_out); $i++) {
    print '&' if ( is_return_value($$in[$i]) && !is_array($$in[$i]) );
    print "arg$i";
    print ', ' unless $i >= ($arity_in + $arity_special_out) - 1;
  }
  print ");\n";

  #
  # Return values
  #
  if ($arity_out != 0) {
    # input arguments which GTK+ uses as return values
    my $ret_number = $arg_number = -1;
    foreach my $arg (@$in) {
      $arg_number++;
      next unless is_return_value($arg);
      $ret_number++;
      $arg =~ s/\*$//s; # drop last asterisk
      print "\tOZ_out($ret_number) = " . c_value2oz_term("arg$arg_number", $arg) . ";\n";
    }
    # the regular return value
    print "\tOZ_out(" . ($arity_out - 1) . ') = ' . c_value2oz_term('ret', $out) . ";\n" if $out;
    print "\treturn OZ_ENTAILED;\n";
  } else {
    print "\treturn OZ_ENTAILED;\n";
  }

  print "} OZ_BI_end\n\n";
}

###############################################################################
#
# gen_c_wrappers
#
# Process a specification and generate glue code for it
#
###############################################################################
sub process_spec {
  return unless ($$class{meths} or $$class{inits});

  my $inits = $$class{inits};
  foreach my $meth (keys %$inits) {
    write_oz_bi_definition($inits, $meth);
  }

  my $meths = $$class{meths};
  foreach my $meth (keys %$meths) {
    write_oz_bi_definition($meths, $meth);
  }
}

###############################################################################
#
# gen_c_wrappers
#
# Generate the C glue code for the given spec files
#
###############################################################################
sub gen_c_wrappers {
  my @specs = @_;

  gen_prelude();

  foreach my $spec (@specs) {
      require $spec;
      process_spec;
  }
}

###############################################################################
#
# gen_gnome_macros
#
# Generate the macro definitions to rename gnome_... and Gnome... to gtk_...
# and Gtk...
#
###############################################################################
sub gen_gnome_macros {
    my @specs = @_;
    local %macros = ();

    foreach my $spec (@specs) {
        require $spec;
        &gnome_process_spec;
    }

    my ($key,$val);
    while (($key,$val) = each %macros) {
        print "#define $key $val\n";
    }
}

sub gnome_save {
    my $name = shift;
    if ($name && $name =~ /^([Gg])nome/) {
        $macros{$name} = (($1 eq 'G')?'Gtk':'gtk') . $';
    }
}

sub gnome_clean_type {
    my ($type_str) = @_;
    if ($type_str) {
        $type_str =~ s/^[\%\!\+\=\^]+//s ;
        $type_str =~ s/const //sg;
        $type_str =~ s/[*]//g;
    }
    return $type_str;
}

sub gnome_process_map {
    my $map = shift;
    my ($key,$val,$typ);
    while (($key,$val) = each %$map) {
        gnome_save($key);
        foreach $typ (@{$val->{'in'}}) {
            gnome_save(gnome_clean_type($typ));
        }
        gnome_save(gnome_clean_type($val->{'out'}));
    }
}

sub gnome_process_spec {
    gnome_save($$class{name});
    gnome_save($$class{super});
    gnome_process_map($$class{'inits'});
    gnome_process_map($$class{'meths'});
}

###############################################################################
#
# usage
#
###############################################################################
sub usage() {
    print <<EOF;
usage: $0 OPTION INPUT_FILE ...

Generate glue code for the GTK+ binding for Oz

  --c-wrappers              generate the C glue code
  --interface               generate the interface definition for
                            Oz C/C++ interface
  --gnome                   generate macros for gnome/gtk canvas renaming
  --includes                Optional header files to include
EOF

    exit 0;
}

###############################################################################
#
# main
#
###############################################################################

# parse arguments
my ($opt_cwrappers,
    $opt_interface,
    $opt_gnome,
    $opt_includes) = (0, 0, 0, 0);
&GetOptions("c-wrappers"     =>    \$opt_cwrappers,
            "interface"      =>    \$opt_interface,
            "gnome"          =>    \$opt_gnome,
            "includes:s"     =>    \$opt_includes);

@input    = @ARGV;
@includes = split /\s/, $opt_includes; # headers to include

usage() unless @input != 0;
usage() unless $opt_cwrappers | $opt_interface | $opt_gnome;

gen_c_wrappers   (@input) if $opt_cwrappers;
gen_oz_interface (@input) if $opt_interface;
gen_gnome_macros (@input) if $opt_gnome;
