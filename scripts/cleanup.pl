#!/usr/bin/perl

if ( $ARGV[0] eq "-h" )
{
	open ( SDIR, "pwd |" );
	$subdir = <SDIR>;
	chop($subdir);
	print("\n\n");
	print("CleanUp -> a handy application for removing old/unused files from a directory\n");
	print("Syntax: cleanup.pl directoy minimum-year minimum-month\n");
	print("Defaults are:\ndirectory($subdir)\nminimum-year(1999)\nminimum-month(11)\n" );
	exit();
}

$subdir = $ARGV[0];
$minyear = $ARGV[1];
$minmonth = $ARGV[2];

if ( $minyear eq "" )
{
	$minyear = 1999;
}

if ( $minmonth eq "" )
{
	$minmonth = 11;
}

if ( $subdir eq "" )
{
	open ( SDIR, "pwd |" );
	$subdir = <SDIR>;
}

 
unless ( open( DIR, "ls -l --full-time $subdir |" ) )
{
	die("Argh that didnt work\n");
}

@files = <DIR>;
$linectr = 3;
%month = 
( 
	"Jan" => 1,
	"Feb" => 2,
	"Mar" => 3,
	"Apr" => 4,
	"May" => 5,
	"Jun" => 6,
	"Jul" => 7,
	"Aug" => 8,
	"Sep" => 9,
	"Oct" => 10,
	"Nov" => 11,
	"Dec" => 12
 );

	while ( $files[$linectr] ne "" )
	{
		$string = $files[$linectr];
		$string =~ /.* (.{3}) \d{2} \d{2}\:\d{2}\:\d{2} ([1-2][0-9]{3}) (.*)/;
		$mon = $1;
		$year = $2;
		$filename = $3;
		$todelete = $subdir."/".$filename;
		if ( ( $year < $minyear ) 
			|| ( $year == $minyear && $month{$mon} < $minmonth))
		{
			system( "rm $todelete" );			
		}

		$linectr++;			

	}

