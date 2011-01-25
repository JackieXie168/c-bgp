<!-- ################################################################
     XSLT transformation from C-BGP doc node to tex

     Copyright (C) 2007-2011, Bruno Quoitin (bruno.quoitin@umons.ac.be)
     ################################################################

     ################################################################
!-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="text" omit-xml-declaration="yes"/>

  <xsl:template name="doc.cmd" match="cmd">
    <xsl:text> {\bf \hyperref[cmd:</xsl:text>
    <xsl:value-of select="link"/>
    <xsl:text>]{</xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text>}}</xsl:text>
  </xsl:template>

  <xsl:template name="doc.arg" match="arg">
    <b>
      <xsl:apply-templates/>
    </b>
  </xsl:template>

  <xsl:template name="tab.for.loop" mode="code">
    <xsl:param name="i"/>
    <xsl:param name="count"/>
    <xsl:if test="$i &lt;= $count">
      <xsl:text disable-output-escaping="yes"> </xsl:text>
    </xsl:if>
    <xsl:if test="$i &lt;= $count">
      <xsl:call-template name="tab.for.loop">
        <xsl:with-param name="i">
          <xsl:value-of select="$i+1"/>
        </xsl:with-param>
        <xsl:with-param name="count">
          <xsl:value-of select="$count"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="tab" mode="code">
    <xsl:call-template name="tab.for.loop">
      <xsl:with-param name="i">1</xsl:with-param>
      <xsl:with-param name="count">
        <xsl:value-of select="2*@count"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="p">
    <!--<xsl:text>\noindent </xsl:text>!-->
    <xsl:apply-templates/>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="br">
    <!--<xsl:text></xsl:text>!-->
  </xsl:template>

  <xsl:template match="i">
    <xsl:text> {\it </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="b">
    <xsl:text> {\bf </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="url">
    <xsl:text disable-output-escaping="yes">\url{</xsl:text>
    <xsl:apply-templates/>
    <xsl:text disable-output-escaping="yes">}</xsl:text>
  </xsl:template>

  <xsl:template match="pre">
    <pre><xsl:apply-templates/></pre>
  </xsl:template>

  <xsl:template match="code">
    <xsl:text>\begin{verbatim}</xsl:text>
    <xsl:apply-templates mode="code"/>
    <xsl:text>\end{verbatim}</xsl:text>
  </xsl:template>

  <xsl:template match="dl">
    <xsl:text>\begin{description}
</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>\end{description}
</xsl:text>
  </xsl:template>

  <xsl:template match="dt">
    <xsl:text>\item[</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>]
</xsl:text>
  </xsl:template>

  <xsl:template match="dd">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="table">

    <xsl:text>%\begin{table}
\begin{center}
\begin{tabular}{|l|l|}
\hline
</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>\hline
</xsl:text>
    <xsl:text>\end{tabular}
\end{center}
%\end{table}</xsl:text>
  </xsl:template>

  <xsl:template name="cell">
    <xsl:param name="final"/>
    <xsl:param name="header"/>

    <xsl:if test="$header = 1">
      <xsl:text>{\bf </xsl:text>
      <xsl:apply-templates/>
      <xsl:text>}</xsl:text>
    </xsl:if>
    <xsl:if test="$header != 1">
      <xsl:apply-templates/>
    </xsl:if>

    <xsl:choose>
      <xsl:when test="$final &gt; 0">
        <xsl:text>&amp;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>\\
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="tr">
    <xsl:variable name="count" select="count(td)"/>
    <xsl:for-each select="td">
      <xsl:call-template name="cell">
        <xsl:with-param name="final" select="$count - position()"/>
        <xsl:with-param name="header" select="0"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="trh">
    <xsl:variable name="count" select="count(th)"/>
    <xsl:for-each select="th">
      <xsl:call-template name="cell">
        <xsl:with-param name="final" select="$count - position()"/>
        <xsl:with-param name="header" select="1"/>
      </xsl:call-template>
    </xsl:for-each>
    <xsl:text>\hline
</xsl:text>
  </xsl:template>

  <xsl:template match="td">
    <xsl:apply-templates/>
    <xsl:text>&amp;
</xsl:text>
  </xsl:template>

  <xsl:template name="arrow.link">
    <xsl:param name="type"/>
    <xsl:text disable-output-escaping="yes">&lt;a href="</xsl:text>
    <xsl:value-of select="id"/>
    <xsl:text disable-output-escaping="yes">.html" style="decoration: none"&gt;</xsl:text>
    <xsl:text disable-output-escaping="yes">&lt;img </xsl:text>
    <xsl:choose>
      <xsl:when test="$type = 0">
        src="../images/16-arrow-left.png"
      </xsl:when>
      <xsl:when test="$type = 1">
        src="../images/16-arrow-up.png"
      </xsl:when>
      <xsl:when test="$type = 2">
        src="../images/16-arrow-right.png"
      </xsl:when>
    </xsl:choose>
    alt="<xsl:value-of select="name"/>" 
    title="<xsl:value-of select="name"/>"
    <xsl:text disable-output-escaping="yes">/&gt;</xsl:text>
    <xsl:text disable-output-escaping="yes">&lt;/a&gt;</xsl:text>
  </xsl:template>

  <xsl:template name="latex.escape">
    <xsl:param name="i"/>
    <xsl:param name="length"/>
    <xsl:variable name="SS">
      <xsl:value-of select="substring(., $length - $i + 1, 1)"/>
    </xsl:variable>
    <xsl:if test="$i &gt;= 0">
      <xsl:choose>
        <xsl:when test="$SS = '_'">
          <xsl:text>\_</xsl:text>
        </xsl:when>
        <xsl:when test="$SS = '&amp;'">
          <xsl:text>\&amp;</xsl:text>
        </xsl:when>
        <xsl:when test="$SS = '|'">
          <xsl:text>$|$</xsl:text>
        </xsl:when>
        <xsl:when test="$SS = '-'">
          <xsl:text>-{}</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$SS"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="latex.escape">
        <xsl:with-param name="i" select="$i - 1"/>
        <xsl:with-param name="length" select="$length"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="latex.escape" mode="code">
    <xsl:value-of select="."/>
  </xsl:template>

  <xsl:template match="text()">
    <xsl:variable name="S">
      <xsl:value-of select="."/>
    </xsl:variable>
    <xsl:call-template name="latex.escape">
      <xsl:with-param name="i" select="string-length($S)"/>
      <xsl:with-param name="length" select="string-length($S)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="/">
    <xsl:text disable-output-escaping="yes">\subsection{</xsl:text>
    <xsl:apply-templates select="command/context"/>
    <xsl:text disable-output-escaping="yes"> </xsl:text>
    <xsl:apply-templates select="command/name"/>
    <xsl:text disable-output-escaping="yes"></xsl:text>
    <xsl:text>}
</xsl:text>
    <xsl:text>\label{cmd:</xsl:text>
    <xsl:value-of select="concat(concat(translate(command/context, ' ', '_'), '_'), translate(command/name, ' ', '_'))"/>
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
