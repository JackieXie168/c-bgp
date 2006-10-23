#!/bin/bash
# ===================================================================
# C-BGP Validation script packaging
# (C) 2006, Bruno Quoitin
# 13/10/2006
# ===================================================================


##### Script configuration ##########################################
#
# VALID_HOME (mandatory)
#   Home directory of the validation script
#
VALID_HOME=/Users/bqu/Documents/code/cbgp/valid
#
# FILE_LIST (mandatory)
#   List of files to be included in the package
#
FILES_LIST=" \
    cbgp-validation.pl \
    CBGP.pm \
    CBGPValid/BaseReport.pm \
    CBGPValid/HTMLReport.pm \
    CBGPValid/TestConstants.pm \
    CBGPValid/Tests.pm \
    CBGPValid/UI.pm \
    abilene-rib.ascii \
    valid-bgp-topology.subramanian \
    valid-record-route.ntf \
    "
#
# TEMP_DIR (mandatory)
#   Temporary directory where the package will be created
#
TMP_DIR=/tmp
#
# PUBLISH_URL
#   Destination of the package (complete SCP "URL")
#
PUBLISH_URL=bqu@midway.info.ucl.ac.be:/home/httpd/cbgp/downloads
#
#####################################################################

    
# -----[ error ]-----------------------------------------------------
# Report a fatal error message and exit
# -------------------------------------------------------------------
error()
{
    MSG=$1
    echo -e "ERROR: \033[1;31m$MSG\033[0m"
    exit -1
}

# -----[ info ]------------------------------------------------------
# Display an information message
# -------------------------------------------------------------------
info()
{
    MSG=$1
    echo -e "\033[1m$MSG\033[0m"
}

# -------------------------------------------------------------------
# Check configuration variables
# -------------------------------------------------------------------
info "Checking configuration..."
if [ -z "$TMP_DIR" ]; then
    error "TMP_DIR variable is not defined"
fi
if [ -z "$FILES_LIST" ]; then
    error "FILES_LIST variable is not defined"
fi
if [ -z "$VALID_HOME" ]; then
    error "VALID_HOME variable is not defined"
fi

# -------------------------------------------------------------------
# Determine validation script's version
# -------------------------------------------------------------------
info "Getting package version..."
VALID_VERSION=`cat $VALID_HOME/cbgp-validation.pl | grep "use constant CBGP_VALIDATION_VERSION" | sed 's/^use constant CBGP_VALIDATION_VERSION \=\> .\([0-9.]*\).;$/\1/'`
if [ $? != 0 ]; then
    error "could not get script version (cbgp-validation.pl)"
fi
if [ -z "$VALID_VERSION" ]; then
    error "could not get script version (empty)"
fi
echo "  version: $VALID_VERSION"
PKG_NAME="cbgp-validation-$VALID_VERSION"
PKG_FILE="$PKG_NAME.tar.gz"
PKG_DIR="$PKG_NAME"

# -------------------------------------------------------------------
# Create temporary package directory
# -------------------------------------------------------------------
mkdir $TMP_DIR/$PKG_DIR
if [ $? != 0 ]; then
    error "could not create temporary package directory"
fi

# -------------------------------------------------------------------
# Copy files to temporary directory
# -------------------------------------------------------------------
info "Packaging files..."
OLDPWD=$PWD
cd $VALID_HOME
for f in $FILES_LIST; do
    cp  --parents $f $TMP_DIR/$PKG_DIR
    if [ $? != 0 ]; then
	error "could not copy \"$f\" to \"$PKG_DIR\""
    fi
done
cd $OLDPWD

# -------------------------------------------------------------------
# Build the archive
# -------------------------------------------------------------------
info "Building archive..."
OLDPWD=$PWD
cd $TMP_DIR
tar cvzf $PKG_FILE $PKG_DIR
if [ $? != 0 ]; then
    error "could not build package archive \"$PKG_FILE\""
fi
cd $OLDPWD

# -----------------------------------------------------------------
# Copy the package to the website
# -----------------------------------------------------------------
if [ ! -z "$PUBLISH_URL" ]; then
    info "Publishing..."
    scp $TMP_DIR/$PKG_FILE $PUBLISH_URL
    if [ $? != 0 ]; then
	error "could not publish package to \"$PUBLISH_URL\""
    fi
fi

# -----------------------------------------------------------------
# Clean temporary files and directory
# -----------------------------------------------------------------
info "Cleaning temporary files..."
rm $TMP_DIR/$PKG_FILE
rm -Rf $TMP_DIR/$PKG_DIR