<!-- ################################################################
     XSLT transformation from C-BGP doc node to groff

     Copyright (C) 2007, Bruno Quoitin (bruno.quoitin@umons.ac.be)
     ################################################################

     INTERNAL DOCUMENTATION:
     There are special modes for elements that can occur within other
     elements and produce groff commands. The first (outer) element
     must generate groff escape characters (basically, a carriage
     return, then "."). While other (inner) commands must not
     generate this escape sequence.

     Inner element template are in a XSLT mode named "in_groff".

     ################################################################
!-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="text" omit-xml-declaration="yes"/>

  <xsl:include href="groff.xsl"/>

  <!-- ##############################################################
       Main transformation program
       ############################################################## !-->
  <xsl:template match="/">
    <xsl:text>.Dt "C-BGP Documentation" "" "User's manual" &#xa;</xsl:text>
    <xsl:for-each select="command">
      <!-- ***********
           NAME
           *********** !-->
      <xsl:text>.Sh NAME&#xa;</xsl:text>
      <xsl:text>.Nm </xsl:text>
      <xsl:value-of select="context"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="name"/>
      <xsl:for-each select="abstract">
        <xsl:text>&#xa;</xsl:text>
        <xsl:text>.Nd </xsl:text>
        <xsl:value-of select="."/>
      </xsl:for-each>
      <xsl:text>&#xa;</xsl:text>
      <!-- ***********
           SYNOPSIS
           *********** !-->
      <xsl:text>.Sh SYNOPSIS&#xa;</xsl:text>
      <xsl:text>.Nm </xsl:text>
      <xsl:value-of select="name"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:for-each select="parameters">
        <xsl:for-each select="option">
          <xsl:text>.Op Fl </xsl:text>
          <xsl:value-of select="name"/>
          <xsl:text>&#xa;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="parameter">
          <xsl:text>.Ao Ar </xsl:text>
          <xsl:value-of select="name"/>
          <xsl:text> Ac&#xa;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="vararg">
          <xsl:text>.Bk&#xa;</xsl:text>
          <xsl:text>.Op Ar </xsl:text>
          <xsl:value-of select="name"/>
          <xsl:text> ... &#xa;</xsl:text>
          <xsl:text>.Ek&#xa;</xsl:text>
        </xsl:for-each>
      </xsl:for-each>
      <!-- ***********
           ARGUMENTS
           *********** !-->
      <xsl:for-each select="parameters">
        <xsl:text>.Sh ARGUMENTS&#xa;</xsl:text>
        <xsl:text>.Bl -ohang&#xa;</xsl:text>
        <xsl:for-each select="option">
          <xsl:text>.It Op Fl </xsl:text>
          <xsl:value-of select="name"/>
          <xsl:text> Ar value&#xa; </xsl:text>
          <xsl:apply-templates select="description"/>
          <xsl:text>&#xa;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="parameter">
          <xsl:text>.It Ao Ar </xsl:text>
          <xsl:value-of select="name"/>
          <xsl:text> Ac </xsl:text>
          <xsl:apply-templates select="description"/>
          <xsl:text>&#xa;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="vararg">
          <xsl:text>.It Op Ar </xsl:text>
          <xsl:value-of select="name"/>
          <!--<xsl:text>)?[</xsl:text><xsl:value-of select="cardinality"/>!-->
          <xsl:text> ...&#xa;</xsl:text>
          <xsl:value-of select="description"/>
          <xsl:text>&#xa;</xsl:text>
        </xsl:for-each>
        <xsl:text>.El&#xa;</xsl:text>
      </xsl:for-each>
      <!-- ***********
           DESCRIPTION
           *********** !-->
      <xsl:for-each select="description">
        <xsl:if test="string-length() > 0">
          <xsl:text>.Sh DESCRIPTION&#xa;</xsl:text>
          <xsl:apply-templates match="code" select="."/>
        </xsl:if>
      </xsl:for-each>
      <!-- ***********
           SUB COMMANDS
           *********** !-->
      <xsl:for-each select="document($filename)/command/childs">
        <xsl:text>.Sh SUB COMMANDS&#xa;</xsl:text>
        <xsl:text>.Bl -bullet -compact&#xa;</xsl:text>
        <xsl:for-each select="child">
          <xsl:text>.It&#xa;.Nm </xsl:text>
          <xsl:value-of select="name"/>
          <xsl:text>&#xa;</xsl:text>
        </xsl:for-each>
        <xsl:text>.El&#xa;</xsl:text>
      </xsl:for-each>
      <!-- ********
           SEE ALSO
           ******** !-->
      <xsl:for-each select="see-also">
        <xsl:if test="string-length() > 0">
          <xsl:text>.Sh SEE ALSO&#xa;</xsl:text>
          <xsl:apply-templates match="code" select="."/>
        </xsl:if>
      </xsl:for-each>
      <!-- ****
           BUGS
           **** !-->
      <xsl:for-each select="bugs">
        <xsl:if test="string-length() > 0">
          <xsl:text>.Sh BUGS&#xa;</xsl:text>
          <xsl:apply-templates match="code" select="."/>
        </xsl:if>
      </xsl:for-each>
      <!-- ***********
           AUTHORS
           *********** !-->
      <xsl:text>.Sh AUTHORS&#xa;</xsl:text>
      <xsl:text>.Pp&#xa;Written by&#xa;.An "Bruno Quoitin" Aq bruno.quoitin@umons.ac.be ,&#xa;Networking Lab, Computer Science Institute, Science Faculty, University of Mons, Belgium.</xsl:text>

    </xsl:for-each>          
    
  </xsl:template>
  
</xsl:stylesheet>
