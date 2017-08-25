#!/usr/bin/perl -w

use strict;
print <<EOD;
/* This is an AUTOGENERATED file from IANA registered headers descpriptions.
 */

#include <pushkin/http/common/grammar/header_generate.hpp>

namespace psst {
namespace http {
namespace grammar {
namespace gen {

known_header_name_grammar::known_header_name_grammar()
{
	add
EOD

while (<>) {
	s/\r\n//g;
	my ($header, $template, $protocol, $status, $reference) = split(',', $_, 5);
	$reference =~ s/"//g;
	$reference =~ s/\n//;
	my $enum_val = $header;
	$enum_val =~ s/-//g;
	
	print "\t\t($enum_val, \"$header\")\n";
}

print <<EOD;
	;
}

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace psst
EOD

