# ===================================================================
# CBGPValid::TestConstants.pm
#
# author Bruno Quoitin (bruno.quoitin@uclouvain.be)
# lastdate 11/06/2009
# ===================================================================

package CBGPValid::TestConstants;

require Exporter;
@ISA= qw(Exporter);
@EXPORT= qw(TEST_FIELD_ID
	    TEST_FIELD_FILE
	    TEST_FIELD_NAME
	    TEST_FIELD_FUNC
	    TEST_FIELD_RESULT
	    TEST_FIELD_DURATION
	    TEST_FIELD_ARGS
	    TEST_FAILURE
	    TEST_SUCCESS
	    TEST_SKIPPED
	    TEST_DISABLED
	    TEST_CRASHED
	    TEST_TODO
	    TEST_RESULT_MSG
	   );

use strict;

use constant TEST_FIELD_ID       => 0;
use constant TEST_FIELD_FILE     => 1;
use constant TEST_FIELD_NAME     => 2;
use constant TEST_FIELD_FUNC     => 3;
use constant TEST_FIELD_RESULT   => 4;
use constant TEST_FIELD_DURATION => 5;
use constant TEST_FIELD_DOC      => 6;
use constant TEST_FIELD_ARGS     => 7;

use constant TEST_FAILURE  => 0;
use constant TEST_SUCCESS  => 1;
use constant TEST_SKIPPED  => 2;
use constant TEST_DISABLED => 3;
use constant TEST_CRASHED  => 4;
use constant TEST_TODO     => 5;

use constant TEST_RESULT_MSG => {
				 TEST_FAILURE()  => "FAILURE",
				 TEST_SUCCESS()  => "SUCCESS",
				 TEST_SKIPPED()  => "SKIPPED",
				 TEST_DISABLED() => "DISABLED",
				 TEST_CRASHED()  => "CRASHED",
				 TEST_TODO()     => "TO-DO",
				};
