<!-- ################################################################
     XSLT transformation from C-BGP doc node to tex

     Copyright (C) 2007-2011, Bruno Quoitin (bruno.quoitin@umons.ac.be)
     ################################################################

     ################################################################
!-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="text" omit-xml-declaration="yes"/>

  <xsl:include href="tex.xsl"/>


  <xsl:template match="/">
    <xsl:text disable-output-escaping="yes">\subsection{</xsl:text>
    <xsl:apply-templates select="command/context"/>
    <xsl:text disable-output-escaping="yes"> </xsl:text>
    <xsl:apply-templates select="command/name"/>
    <xsl:text disable-output-escaping="yes"></xsl:text>
    <xsl:text>}
</xsl:text>
    <xsl:text>\label{cmd:</xsl:text>
    <xsl:if test="string(command/context)">
      <xsl:value-of select="concat(translate(command/context, ' ', '_'), '_')"/>  
    </xsl:if>
    <xsl:value-of select="translate(command/name, ' ', '_')"/>
    <xsl:text>}
</xsl:text>

    <xsl:text>\subsubsection*{Abstract}
</xsl:text>
    <xsl:value-of select="command/abstract"/>


                <xsl:text>
\subsubsection*{Synopsis}
{\tt </xsl:text>
                  <xsl:value-of select="command/name"/>
                  <xsl:for-each select="command/parameters">
                    <xsl:for-each select="option">
                      <xsl:text> [</xsl:text>
                      <xsl:value-of select="name"/>
                      <xsl:text>]</xsl:text>
                    </xsl:for-each>
                    <xsl:for-each select="parameter">
                      <xsl:text> &lt;</xsl:text>
                      <xsl:value-of select="name"/>
                      <xsl:text>&gt;</xsl:text>
                      <xsl:text disable-output-escaping="yes"> </xsl:text>
                    </xsl:for-each>
                    <xsl:for-each select="vararg">
                      <xsl:text>&lt;</xsl:text>
                      <xsl:value-of select="name"/>
                      <xsl:text>&gt;</xsl:text>?[<xsl:value-of select="cardinality"/>]
                      <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
                    </xsl:for-each>
                  </xsl:for-each>
                  <xsl:text>}</xsl:text>

                <xsl:for-each select="command/parameters">
                  <xsl:if test="string(.)">
                    <xsl:text>\subsubsection*{Arguments}
\begin{description}
</xsl:text>
<xsl:for-each select="option">
  <xsl:text>  \item[</xsl:text>
                    <xsl:apply-templates select="name"/>
                    <xsl:text>]</xsl:text>
                    <xsl:apply-templates select="description"/>
                    <xsl:text>
                    </xsl:text>
                  </xsl:for-each>
                  <xsl:for-each select="parameter">
                    <xsl:text>  \item[</xsl:text>
                    <xsl:apply-templates select="name"/>
                    <xsl:text>]</xsl:text>
                    <xsl:apply-templates select="description"/>
                    <xsl:text>
                    </xsl:text>
                  </xsl:for-each>
                  <xsl:for-each select="vararg">
                    <xsl:text>  \item[</xsl:text>
                    <xsl:apply-templates select="name"/>
                    [<xsl:value-of select="cardinality"/>]
                    <xsl:text>]</xsl:text>
                    <xsl:apply-templates select="description"/>
                    <xsl:text>
                    </xsl:text>
                  </xsl:for-each>
                  <xsl:text>\end{description}
</xsl:text>
                  </xsl:if>
                </xsl:for-each>

                <xsl:for-each select="command/description">
                  <xsl:text>\subsubsection*{Description}
</xsl:text>
                   <xsl:choose>
                    <xsl:when test="string(.)">
                     <xsl:apply-templates match="code" select="."/>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:text>No description is available.</xsl:text>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:for-each>

                <!--<xsl:for-each select="document($filename)/command/childs">
                  <h4>Sub-commands</h4>
                  <p>
                    <ul>
                      <xsl:for-each select="child">
                        <li>
                          <xsl:text disable-output-escaping="yes">&lt;a href="</xsl:text>
                          <xsl:value-of select="id"/>
                          <xsl:text disable-output-escaping="yes">.html"&gt;</xsl:text>
                          <xsl:value-of select="name"/>
                          <xsl:text disable-output-escaping="yes">&lt;/a&gt;</xsl:text>
                        </li>
                      </xsl:for-each>
                    </ul>
                  </p>
                </xsl:for-each>!-->

                <xsl:for-each select="command/see-also">
                  <xsl:text>\subsubsection*{See also}
</xsl:text>
                  <xsl:apply-templates select="."/>
                </xsl:for-each>

                <xsl:for-each select="command/bugs">
                  <xsl:text>\subsubsection*{Known bugs}
</xsl:text>
                  <xsl:apply-templates select="."/>
                </xsl:for-each>

  </xsl:template>

</xsl:stylesheet>
