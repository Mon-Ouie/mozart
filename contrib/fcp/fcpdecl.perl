#! /usr/local/bin/perl

###
### Authors:
###   Denys Duchier <duchier@ps.uni-sb.de>
###   Leif Kornstaedt <kornstae@ps.uni-sb.de>
###   Michael Mehl <mehl@dfki.de>
###   Christian Schulte <schulte@ps.uni-sb.de>
###
### Copyright:
###   Denys Duchier, 1998
###   Leif Kornstaedt, 1998
###   Michael Mehl, 1998
###   Christian Schulte, 1998
###
### Last change:
###   $Date$ by $Author$
###   $Revision$
###
### This file is part of Mozart, an implementation
### of Oz 3:
###    http://www.mozart-oz.org
###
### See the file "LICENSE" or
###    http://www.mozart-oz.org/LICENSE.html
### for information on usage and redistribution
### of this file, and for a DISCLAIMER OF ALL
### WARRANTIES.
###


###
### Here we declare all builtins in a general format which can be used
### to generate both the table of builtins used by the emulator and the
### information used by the Oz compiler.
###
###     bidecl.perl -interface
###             generates the table of builtins for the emulator
###     bidecl.perl -oztable
###             generates the table of builtins for the Oz compiler
###     bidecl.perl -builtins Foo Bar Baz
###             generates the table of builtins for the compiler
###             using files modFoo.spec modBar.spec modBaz.spec
###             and for each one: adds the appropriate module prefix.
###             thus a builtin bi in Foo is renamed Foo.bi
###
### ADDITIONAL OPTIONS
###
###     -include M1,M2,...,Mn
###     -exclude M1,M2,...,Mn
###
###             include (resp. exclude) only these modules. by default
###     all modules are included.  Only one of these options may be
###     present (actually there can be several of them as long as they
###     are all -include or all -exclude.  Their arguments are unioned).
###
### Each entry has the form: 'NAME' => { ... }, where 'NAME' is the
### string by which the builtin is known to the emulator.  The associative
### array describing the builtin contains at least:
###     in  => [...],
###     out => [...],
### describing respectively the input and output arguments.  Each argument
### is described by a type, possibly annotated by a prefix indicating
### the determinacy condition.  Thus an integer argument might be specified
### in one of these ways:
###      'int'          possibly non-determined
###     '+int'          determined
###     '*int'          kinded (e.g. an FD variable)
### '+int' (resp. '*int') indicates that the builtin will suspend until
### this argument is determined (resp. kinded).
###
### Furthermore, there are builtins that overwrite their input arguments.
### This should be indicated by the prefix `!'. Thus '!+value' indicates
### an argument for which the builtin will suspend until it is determined
### and which may be overwriten by its execution.
###
### The annotations +,* and ! may be given in arbitrary order.
###
### A type may be simple or complex:
###
### SIMPLE    ::= ... any atom that occurs as feature in the Type.is module ...
###
### COMPLEX   ::= [SIMPLE]              (list of SIMPLE)
###             | [SIMPLE#SIMPLE]       (list of pairs of SIMPLE)
###             |  SIMPLE#SIMPLE        (pair or SIMPLE)
###
### determinacy annotations for subtypes of complex types are not yet
### supported.
###
### Old style builtins have: bi => OLDBI, where OLDBI is the name
### of the C procedure that implements it (normally defined using
### OZ_C_proc_begin(OLDBI,...)).
###
### New style builtins have: BI => NEWBI, where NEWBI is the name
### of the C procedure that implements it (defined using
### OZ_BI_define(NEWBI,...,...)).
###
### ifdef => MACRO, indicates that the entry for this builtin in the
### emulator's table should be bracketed by #ifdef MACRO ... #endif.
### Actually MACRO can be of the form M1,M2,...,Mn in which case there
### will be n bracketing, one for each macro M1 to Mn.
###
### ifndef => M1,...,Mn is similar for #ifndef ... #endif.  both ifdef
### and ifndef may be present.
###
### doesNotReturn => 1, indicates that the builtin does not return and
### therefore that the code following it will never be executed.  For
### example 'raise'.
###
### negated => BI, indicates that the builtin BI returns the negated
### result of the builtin.  For example `<' is the negated version of `>='.
### This only makes sense for builtins with a single output argument which
### is of type bool.
###
### module => M, indicates that the builtin belongs to module M.  This
### permits selective inclusion or exclusion through command line options
### -include or -exclude.
###
### In case the variable `module_init_fun_name' is defined in the module
### specification (e.g. `$module_init_fun_name = "fdp_init";' in modFDP.spec),
### the appropriate call to this function will be included in the module
### initialization function. That allows to initialize modules explicitely.



sub FUNCTOR {
    $mod_name = $_[0];
    $module_name   = $_[2];

    $import_module_name = $module_name;
    if ($module_name eq "FDP") {
        $module_name = "FD";
    }
    if ($module_name eq "FSP") {
        $module_name = "FS";
    }

    print "\n/* This file is automatically generated.         */";
    print "\n/* Do not edit this file.                        */";
    print "\n/* Copyright Tobias M�ller tmueller2ps.uni-sb.de */\n\n";

    print "\nfunctor\n";
    print "\nexport\n";

    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        $ignore = $info->{fcp} || dontignore;
        if ($ignore  eq "ignore") {
            next;
        }
        $bi = $info->{bi} || $info->{BI};
        my $inArity = @{$info->{in}};
        my $outArity = @{$info->{out}};
        printf ("\t'%s.%s' : `%s`\n", $module_name, $key, $key);
    }

    printf ("\n\tinactive : Inactive\n");

    print "\nimport\n";

    printf ("\tFCPExport at 'mod%s.fcp.so{native}'\n", $import_module_name);
    printf ("\t%s\n", $module_name);
    print "\n\tReflect\n";

    print "\ndefine\n";

    printf ("\t{Wait %s}\n", $module_name);
    print "\t{Wait Reflect}\n\n";


    $mod_name =~ s/^.*\///o;
    $mod_name =~ s/^mod//o;
    $mod_name =~ s/\.spec//o;

    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        $ignore = $info->{fcp} || dontignore;
        if ($ignore  eq "ignore") {
            next;
        }
        $bi = $info->{bi} || $info->{BI};
        my $inArity = @{$info->{in}};
        my $outArity = @{$info->{out}};
        printf ("\t`%s` = FCPExport.'fcp.%s'\n", $key, $key);
    }

    printf ("\tInactive = inactive(\n");
    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        $ignore = $info->{fcp} || dontignore;
        if ($ignore  eq "ignore") {
            next;
        }
        $bi = $info->{bi} || $info->{BI};
        my $inArity = @{$info->{in}};
        my $outArity = @{$info->{out}};
        printf ("\t\t'%s.%s' : FCPExport.'fcp.%s.inactive'\n",
                $module_name, $key, $key);
    }
    printf ("\t)\n");

    print "\nend\n";}

sub INTERFACE {
    $mod_name = $_[0];
    $module_name = $_[2];

    print "\n/* This file is automatically generated. */";
    print "\n/* Do not edit this file.                */";
    print "\n/* Copyright Tobias M�ller tmueller2ps.uni-sb.de */\n\n";

    print "#include \"fcp.hh\"\n";

    print "\n/* PROTOTYPES */\n\n";

    my ($key,$info,$bi);
    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        $ignore = $info->{fcp} || dontignore;
        if ($ignore  eq "ignore") {
            next;
        }
        $bi = $info->{bi} || $info->{BI};
        print "OZ_BI_proto($bi);\n";
    }

    if ($module_name) {
        printf ("\nchar oz_module_name[] = \"%s\";\n\n", $module_name);
    }

    print "\n/* DEFINITION OF FIRST-CLASS PROPAGATORS */\n\n";

    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        $bi = $info->{bi} || $info->{BI};
        $ignore = $info->{fcp} || dontignore;
        if ($ignore  eq "ignore") {
            next;
        }
        my $inArity = @{$info->{in}};
        my $outArity = @{$info->{out}};
        printf ("FIRST_CLASS_PROPAGATOR_OF($bi, fcp_$bi, %d, %d);\n",
                $inArity+1, $outArity);
    }

    $init_fun_name = $_[1];

    if ($init_fun_name) {
        print ("\nvoid $init_fun_name(void);\n\n");
    }

    $mod_name = $_[0];

    $mod_name =~ s/^.*\///o;
    $mod_name =~ s/^mod//o;
    $mod_name =~ s/\.spec//o;

    print ("extern \"C\"\n\{\n");
    print ("  OZ_C_proc_interface * oz_init_module(void)\n");
    print ("  {\n");
    print ("    static OZ_C_proc_interface i_table\[\] = \{\n");

    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        $ignore = $info->{fcp} || dontignore;
        if ($ignore  eq "ignore") {
            next;
        }
        my $inArity = @{$info->{in}};
        my $outArity = @{$info->{out}};
        my $BI = $info->{BI};
        my @ifdef  = split(/\,/,$info->{ifdef});
        my @ifndef = split(/\,/,$info->{ifndef});
        my $macro;
        foreach $macro (@ifdef)  { print "#ifdef $macro\n"; }
        foreach $macro (@ifddef) { print "#ifndef $macro\n"; }
        $BI = $info->{bi} unless $BI;
        printf ("      {\"fcp.$key\", %d, %d, fcp_$BI},\n",
                $inArity+1, $outArity);
        printf ("      {\"fcp.${key}.inactive\", %d, %d, fcp_${BI}_inactive},\n",
                $inArity+1, $outArity);
        foreach $macro (@ifddef) { print "#endif\n"; }
        foreach $macro (@ifdef)  { print "#endif\n"; }
    }

    print ("      {0,0,0,0}\n");
    print ("    \};\n\n");

    if ($init_fun_name) {
        print ("    $init_fun_name();\n\n");
    }

    print ("    return i_table;\n");
    print ("  \} /* oz_init_module */\n");
    print ("\} /* extern \"C\" */\n\n");

}

sub argspec {
    my $spec = shift;
    my ($mod,$det,$typ) = (0,'any','value');
    my $again = 1;

    # first we handle the various annotations

    while ($again) {
        # is the argument register side effected?
        if    ($spec =~ /^\!/) { $spec=$'; $mod=1; }
        # what is the determinacy condition on the argument?
        elsif ($spec =~ /^\+/) { $spec=$'; $det='det'; }
        elsif ($spec =~ /^\*/) { $spec=$'; $det='detOrKinded'; }
        else { $again=0; }
    }

    # now parse the type of the argument

    if    ($spec =~ /^\[(.+)\#(.+)\]$/) { $typ="list(pair('$1' '$2'))"; }
    elsif ($spec =~ /^\[(.+)\]$/      ) { $typ="list('$1')"; }
    elsif ($spec =~ /^(.+)\#(.+)$/    ) { $typ="pair('$1' '$2')"; }
    else                                { $typ="'$spec'"; }

    return ($mod,$det,$typ);
}

sub OZTABLE {
    my ($key,$info);
    while (($key,$info) = each %$builtins) {
        next unless &included($info);
        my (@imods,@idets,@ityps,$spec,$destroys);
        foreach $spec (@{$info->{in}}) {
            my ($mod,$det,$typ) = &argspec($spec);
            $destroys=1 if $mod;
            push @imods,($mod?'true':'false');
            push @idets,$det;
            push @ityps,$typ;
        }
        my (@odets,@otyps);
        foreach $spec (@{$info->{out}}) {
            my ($mod,$det,$typ) = &argspec($spec);
            $det="any(det)" if $det eq 'det';
            push @odets,$det;
            push @otyps,$typ;
        }
        print "'$key':\n\tbuiltin(\n";
        if ((@ityps+@otyps)>0) {
            print "\t\ttypes: [",join(' ',@ityps,@otyps),"]\n";
            print "\t\tdet: [",join(' ',@idets,@odets),"]\n";
        } else {
            print "\t\ttypes: nil\n";
            print "\t\tdet: nil\n";
        }

        if (@imods) {
            print "\t\timods: [",join(' ',@imods),"]\n";
        } else {
            print "\t\timods: nil\n";
        }

        if (exists $info->{test}) {
            print "\t\ttest: true\n" if $info->{test};
        } elsif ($otyps[0] eq '\'bool\'' && $odets[0] eq 'any(det)') {
            print "\t\ttest: true\n";
        }

        print "\t\tdoesNotReturn: true\n" if $info->{doesNotReturn};
        my $negated = $info->{negated};
        print "\t\tnegated: '$negated'\n" if $negated;
        print "\t)\n";
    }
}

sub BUILTINS {
    foreach $mod (@ARGV) {
        require "$srcdir/mod$mod.spec";
        $builtins = {};
        while (($k,$v) = each %builtins_all) {
            $k = "\\'$k\\'" unless $k =~ /^[a-zA-Z]/;
            if (exists $v->{negated}) {
                my $neg = $v->{negated};
                if ($neg !~ /^[a-zA-Z]/) {
                    $neg = "\\'$neg\\'";
                }
                $v->{negated} = "$mod.$neg";
            }
            $builtins->{"$mod.$k"} = $v;
        }
        &OZTABLE;
    }
    exit 0;
}

my %include = ();
my %exclude = ();
my $includedefault = 1;

sub included {
    my $info = shift;
    my $module = $info->{module} || 'oz';
    return 0 if $exclude{$module};
    return 1 if $include{$module};
    return $includedefault;
}

my ($option,$choice,@include,@exclude);

while (@ARGV) {
    $option = shift;
    if    ($option eq '-interface' ) { $choice='interface';  }
    elsif ($option eq '-functor' )   { $choice='functor';  }
    elsif ($option eq '-oztable')    { $choice='oztable'; }
    elsif ($option eq '-include')    { push @include,split(/\,/,shift); }
    elsif ($option eq '-exclude')    { push @exclude,split(/\,/,shift); }
    elsif ($option eq '-file')       { push @files,shift; }
    elsif ($option eq '-srcdir')     { $srcdir=shift; }
    elsif ($option eq '-builtins')   {  &BUILTINS; }
    else { die "unrecognized option: $option"; }
}

if (@include!=0 && @exclude!=0) {
    die "cannot have both -include and -exclude";
}

foreach $option (@include) { $include{$option} = 1; }
foreach $option (@exclude) { $exclude{$option} = 1; }

$includedefault = 0 if @include!=0;

foreach $file (@files) {
    require $file;
    $builtins = { %builtins_all };
    $init_fun_name = $module_init_fun_name;
    if ($boot_module_name) {
        $module_name = $boot_module_name;
    }

    if    ($choice eq 'interface' ) { &INTERFACE($file, $init_fun_name, $module_name); }
    elsif ($choice eq 'functor' )   { &FUNCTOR($file, $init_fun_name, $module_name); }
    elsif ($choice eq 'oztable')    { &OZTABLE; }
    else { die "must specify one of: -functor -interface -oztable -builtins"; }
}
