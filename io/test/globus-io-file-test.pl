#!/usr/bin/env perl

=pod

=head1 Tests for the globus IO file code

    Tests to exercise the file IO functionality of the globus IO library.

=cut

use strict;
use POSIX;
use Test;

my $test_prog = './globus_io_file_test';
my $diff = 'diff';
my @tests;
my @todo;


sub basic_func
{
    my ($errors,$rc) = ("",0);
    
    $rc = system("$test_prog 1>$test_prog.log.stdout 2>$test_prog.log.stderr") / 256;

    if($rc != 0)
    {
        $errors .= "Test exited with $rc. ";
    }

    if(-r 'core')
    {
        $errors .= "\n# Core file generated.";
    }
    
    $rc = system("$diff $test_prog.log.stdout /etc/group") / 256;
    
    if($rc != 0)
    {
        $errors .= "Test produced unexpected output, see $test_prog.log.stdout";
    }

    if( -s "$test_prog.strderr")
    {
        $errors .= "Test produced unexpected output, see $test_prog.log.stderr";
    }
    
    if($errors eq "")
    {
        if( -e "$test_prog.log.stdout" )
        {
            unlink("$test_prog.log.stdout");
        }
        
        if( -e "$test_prog.log.stderr" )
        {
            unlink("$test_prog.log.stderr");
        } 
        ok('success', 'success');
    }
    else
    {
        ok($errors, 'success');
    }
}

sub sig_handler
{
    if( -e "$test_prog.log.stdout" )
    {
        unlink("$test_prog.log.stdout");
    }

    if( -e "$test_prog.log.stderr" )
    {
        unlink("$test_prog.log.stderr");
    }
}

$SIG{'INT'}  = 'sig_handler';
$SIG{'QUIT'} = 'sig_handler';
$SIG{'KILL'} = 'sig_handler';

push(@tests, "basic_func();");

# Now that the tests are defined, set up the Test to deal with them.
plan tests => scalar(@tests), todo => \@todo;

# And run them all.
foreach (@tests)
{
    eval "&$_";
}
