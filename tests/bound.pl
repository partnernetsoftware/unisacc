#!/usr/bin/perl
# Own one process group; bound the command and all descendants, not only exec.
use strict;
use warnings;
use POSIX qw(setsid);
my $seconds = shift @ARGV;
die "bound: timeout must be 1..60 seconds, followed by a command\n"
    unless defined($seconds) && $seconds =~ /\A[0-9]+\z/ && $seconds >= 1 && $seconds <= 60 && @ARGV;
pipe(my $ready, my $notify) or die "pipe: $!\n";
my $pid = fork();
die "fork: $!\n" unless defined $pid;
if (!$pid) {
    close $ready;
    setsid() >= 0 or die "setsid: $!\n";
    print {$notify} "ready\n"; close $notify;
    exec @ARGV;
    die "exec: $!\n";
}
close $notify;
my $stop = sub {
    my ($status) = @_;
    # Nested watchdogs can create new sessions. Freeze the owned process tree
    # before killing it, rather than assuming every child kept our group.
    my %owned = ($pid => 1);
    kill 'STOP', $pid;
    while (1) {
        my $added = 0;
        open(my $ps, '-|', 'ps', '-axo', 'pid=,ppid=') or die "ps: $!\n";
        my @rows = <$ps>; close $ps;
        for my $row (@rows) {
            my ($child, $parent) = $row =~ /^\s*(\d+)\s+(\d+)/;
            next unless defined($parent) && $owned{$parent} && !$owned{$child};
            $owned{$child} = 1; kill 'STOP', $child; $added++;
        }
        last unless $added;
    }
    kill 'KILL', -$pid;
    kill 'KILL', keys %owned;
    waitpid($pid, 0);
    exit $status;
};
$SIG{ALRM} = sub { $stop->(142) };
$SIG{INT} = sub { $stop->(130) };
$SIG{TERM} = sub { $stop->(143) };
alarm $seconds;
my $started = <$ready>; close $ready;
waitpid($pid, 0);
my $status = $?;
alarm 0;
exit(($status & 127) ? 128 + ($status & 127) : $status >> 8);
