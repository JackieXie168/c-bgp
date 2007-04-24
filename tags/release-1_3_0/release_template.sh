#!/bin/bash
# ===================================================================
# Release script for c-bgp
# Last modified: 29/09/2006 (bqu)
# ===================================================================

. release_functions

#####################################################################
# ------------->>>> PARAMETERS UPDATE NEEDED HERE <<<<---------------
#####################################################################
#
# MODULE (mandatory)
#   is the module name (or the project name)
#
MODULE="cbgp"
#
# VC (mandatory)
#   is the version control system (cvs / svn)
#
VC=cvs
#
# SVN_URL (optional)
#   is the SVN URL of the project (this needs only be configured
#   for SVN projects)
#
#SVN_URL=
#
# MAIN_VERSION (mandatory)
#   is the project main version (major)
#
MAIN_VERSION=1
#
# RELEASE_VERSION (mandatory)
#   is the release main version (minor)
#
RELEASE_VERSION=2
#
# BUILD_VERSION (mandatory)
#   is the build number
#
BUILD_VERSION=5
#
# VERSION
#   
VERSION="$MAIN_VERSION.$RELEASE_VERSION.$BUILD_VERSION"
#
# CONF_OPTIONS[] (mandatory)
#   is an array of configure options. The project is built once
#   for each element of the array.
#   Use single quote for no option (otherwise, bash keeps the previous option)
#
CONF_OPTIONS[0]=' '
CONF_OPTIONS[1]="--enable-experimental"
CONF_OPTIONS[2]="--with-jni"
#
# CONF_OPTIONS_COMMON (optional)
#   is a set of common configure options
#                            
CONF_OPTIONS_COMMON="--with-libgds-dir=$HOME/local --with-readline=/sw"
#
# VALID_EXEC (optional)
#   is the name of a command used to validate the project.
#
VALID_EXEC="PERLLIB=/Users/vvandens/Documents/Boulot/svn/CBGP/cbgp-validation-1.6.1:$PERLLIB \
    /Users/vvandens/Documents/Boulot/svn/CBGP/cbgp-validation-1.6.1/cbgp-validation.pl \
    --no-cache \
    --cbgp-path=src/cbgp \
    --resources-path=valid \
    --report=html \
    --report-prefix=$MODULE-$VERSION-valid"
#
# PUBLISH_URL (optional)
#   is the SCP URL where the distribution will be published
#
PUBLISH_URL="vvandens@midway.info.ucl.ac.be:/home/httpd/cbgp/downloads"
#
# PUBLISH_FILES (optional)
#   is the list of files that needs to be published
#
PUBLISH_FILES="$MODULE-$VERSION.tar.gz \
    $MODULE-$VERSION-valid.html \
    $MODULE-$VERSION-valid-doc.html"
#
#####################################################################
# ----->>>> YOU SHOULDN'T CHANGE THE SCRIPT PAST THIS LINE <<<<------
#####################################################################

make_release $@
