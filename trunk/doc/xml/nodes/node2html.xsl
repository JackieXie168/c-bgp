<!-- ################################################################
     XSLT transformation from C-BGP doc node to html

     Copyright (C) 2007, Bruno Quoitin (bruno.quoitin@umons.ac.be)
     ################################################################

     ################################################################
!-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="html" indent="yes"/>

  <xsl:template name="doc.cmd" match="cmd">
    <b>
      <xsl:text disable-output-escaping="yes">&lt;a href="</xsl:text>
      <xsl:value-of select="link"/>
      <xsl:text disable-output-escaping="yes">.html" style="text-decoration: none"&gt;</xsl:text>
      <xsl:value-of select="name">
        <xsl:apply-templates/>
      </xsl:value-of>
      <xsl:text disable-output-escaping="yes">&lt;/a&gt;</xsl:text>
    </b>
  </xsl:template>

  <xsl:template name="doc.arg" match="arg">
    <b>
      <xsl:apply-templates/>
    </b>
  </xsl:template>

  <xsl:template name="tab.for.loop">
    <xsl:param name="i"/>
    <xsl:param name="count"/>
    <xsl:if test="$i &lt;= $count">
      <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
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

  <xsl:template match="tab">
    <xsl:call-template name="tab.for.loop">
      <xsl:with-param name="i">1</xsl:with-param>
      <xsl:with-param name="count">
        <xsl:value-of select="2*@count"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="p">
    <p><xsl:apply-templates/></p>
  </xsl:template>

  <xsl:template match="br">
    <br><xsl:apply-templates/></br>
  </xsl:template>

  <xsl:template match="i">
    <i><xsl:apply-templates/></i>
  </xsl:template>

  <xsl:template match="b">
    <b><xsl:apply-templates/></b>
  </xsl:template>

  <xsl:template match="url">
    <xsl:text disable-output-escaping="yes">&lt;a href="</xsl:text>
    <xsl:apply-templates/>
    <xsl:text disable-output-escaping="yes">"&gt;</xsl:text>
    <xsl:apply-templates/>
    <xsl:text disable-output-escaping="yes">&lt;/a&gt;</xsl:text>
  </xsl:template>

  <xsl:template match="pre">
    <pre><xsl:apply-templates/></pre>
  </xsl:template>

  <xsl:template match="code">
    <code><xsl:apply-templates/></code>
  </xsl:template>

  <xsl:template match="dl">
    <dl><xsl:apply-templates/></dl>
  </xsl:template>

  <xsl:template match="dt">
    <dt><xsl:apply-templates/></dt>
  </xsl:template>

  <xsl:template match="dd">
    <dd><xsl:apply-templates/></dd>
  </xsl:template>

  <xsl:template match="table">
    <center><table><xsl:apply-templates/></table></center>
  </xsl:template>

  <xsl:template match="th">
    <th><xsl:apply-templates/></th>
  </xsl:template>

  <xsl:template match="tr">
    <tr><xsl:apply-templates/></tr>
  </xsl:template>

  <xsl:template match="td">
    <td><xsl:apply-templates/></td>
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

  <xsl:template match="/">
    <xsl:text disable-output-escaping="yes">&lt;!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/&gt;</xsl:text>
    <html xmlns="http://www.w3.org/1999/xhtml">
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=us-ascii"></meta>
        <meta name="Robots" content="index,follow"></meta>
        <meta name="author" content="Bruno Quoitin, bruno.quoitin@umons.ac.be"></meta>
        <link rel="stylesheet" type="text/css" href="../css/style.css"></link>
        <title>C-BGP's documentation</title>
      </head>
      <body>
        <div id="head">
          <div id="title">
            C-BGP
          </div>
        </div>
        <div id="body_wrapper">
          <div id="body">
            <div id="all">
              <div class="top"></div>
              <div class="content">
                <h1>Documentation</h1>

                <center>
                  
                  <xsl:for-each select="document($filename)/command/prev">
                    <xsl:call-template name="arrow.link">
                      <xsl:with-param name="type" select="0"/>
                    </xsl:call-template>
                  </xsl:for-each>
                  <xsl:for-each select="document($filename)/command/father">
                    <xsl:call-template name="arrow.link">
                      <xsl:with-param name="type" select="1"/>
                    </xsl:call-template>
                  </xsl:for-each>
                  <xsl:for-each select="document($filename)/command/next">
                    <xsl:call-template name="arrow.link">
                      <xsl:with-param name="type" select="2"/>
                    </xsl:call-template>
                  </xsl:for-each>
                  <a href="../index.html"><img src="../images/16-file-page.png" alt="index" title="index"/></a>
                </center>

                <h4>Name</h4>
                <p>
                  <xsl:value-of select="command/context"/>
                  <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
                  <b>
                    <xsl:value-of select="command/name"/>
                  </b>
                  <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>-
                  <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
                  <xsl:value-of select="command/abstract"/>
                </p>
                
                <h4>Synopsis</h4>
                <p>
                  <code>
                    <xsl:value-of select="command/name"/>
                    <xsl:text> </xsl:text>
                    <xsl:for-each select="command/parameters">
                      <xsl:for-each select="option">
                        [<xsl:value-of select="name"/>]
                      </xsl:for-each>
                      <xsl:for-each select="parameter">
                        <xsl:text>&lt;</xsl:text>
                        <xsl:value-of select="name"/>
                        <xsl:text>&gt;</xsl:text>
                        <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>

                      </xsl:for-each>
                      <xsl:for-each select="vararg">
                        <xsl:text>&lt;</xsl:text>
                        <xsl:value-of select="name"/>
                        <xsl:text>&gt;</xsl:text>?[<xsl:value-of select="cardinality"/>]
                        <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
                      </xsl:for-each>
                    </xsl:for-each>
                  </code>
                </p>

                <xsl:for-each select="command/parameters">
                  <h4>Arguments</h4>
                  <p>
                    <dl>
                      <xsl:for-each select="option">
                        <dt><xsl:value-of select="name"/></dt>
                        <dd><xsl:value-of select="description"/></dd>
                      </xsl:for-each>
                      <xsl:for-each select="parameter">
                        <dt><xsl:value-of select="name"/></dt>
                        <dd><xsl:value-of select="description"/></dd>
                      </xsl:for-each>
                      <xsl:for-each select="vararg">
                        <dt>
                          <xsl:value-of select="name"/>
                          [<xsl:value-of select="cardinality"/>]
                        </dt>
                        <dd><xsl:value-of select="description"/></dd>
                      </xsl:for-each>
                    </dl>
                  </p>
                </xsl:for-each>

                <xsl:for-each select="command/description">
                  <h4>Description</h4>
                  <xsl:apply-templates match="code" select="."/>
                </xsl:for-each>

                <xsl:for-each select="document($filename)/command/childs">
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
                </xsl:for-each>

                <xsl:for-each select="command/see-also">
                  <h4>See also</h4>
                  <xsl:apply-templates match="code" select="."/>
                </xsl:for-each>

              </div>
              <div class="bottom"></div>
            </div>
            <div class="clearer"></div>
          </div>
          <div class="clearer"></div>
        </div>
        <div id="end_body"></div>
        <div id="footer">
          <xsl:text disable-output-escaping="yes">&amp;copy;</xsl:text> Copyright Bruno Quoitin 2007
        </div>
      </body>
    </html>
  </xsl:template>

</xsl:stylesheet>
