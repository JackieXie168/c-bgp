<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
<xsl:output method="xml"/>
<xsl:template match="/">
  <xsl:text disable-output-escaping="yes">&lt;command&gt;
  </xsl:text>
  <xsl:copy-of select="command/childs"/>
  <xsl:text>
  </xsl:text>
  <xsl:copy-of select="command/father"/>
  <xsl:text disable-output-escaping="yes">
&lt;/command&gt;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
