<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="html"/>

  <xsl:template match="/">
    <html>
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=us-ascii"></meta>
        <meta name="Robots" content="index,follow"></meta>
        <meta name="author" content="Bruno Quoitin, bruno.quoitin@umons.ac.be"></meta>
        <link rel="stylesheet" type="text/css" href="css/style.css"></link>
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

                <h4>Index</h4>
                <p>
                  <xsl:for-each select="docindex/index">
                    <xsl:text disable-output-escaping="yes">&lt;a href="nodes/</xsl:text>
                    <xsl:value-of select="id"/>
                    <xsl:text disable-output-escaping="yes">.html"&gt;</xsl:text>
                    <xsl:value-of select="name"/>
                    <xsl:text disable-output-escaping="yes">&lt;/a&gt;</xsl:text>
                    <br/>
                  </xsl:for-each>
                </p>

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
