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

  <!-- Text is normalized (by default) !-->
  <xsl:template match="text()">
    <xsl:choose>
      <xsl:when test="substring(current(),1,1) = '.'">
        <xsl:text>\).</xsl:text>
        <xsl:value-of select="substring(current(),2)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="normalize-space()"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Text is normalized (by default) !-->
  <xsl:template match="text()" mode="in_groff">
    <xsl:value-of select="normalize-space()"/>
  </xsl:template>

  <!-- Non-normalized text for <code/> element !-->
  <xsl:template match="text()" mode="keep-spaces">
    <xsl:value-of select="."/>
  </xsl:template>

  <!-- <cmd/> element !-->
  <xsl:template match="cmd">
    <xsl:text>&#xa;.Sy "</xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text>"&#xa;</xsl:text>
  </xsl:template>

  <!-- <cmd/> element, in groff mode !-->
  <xsl:template match="cmd" mode="in_groff">
    <xsl:text>Sy "</xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text>" </xsl:text>
  </xsl:template>

  <!-- <arg/> element !-->
  <xsl:template match="arg">
    <xsl:text>&#xa;.Ar </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <!-- <arg/> element, in groff mode !-->
  <xsl:template match="arg" mode="in_groff">
    <xsl:text>Ar </xsl:text>
    <xsl:apply-templates mode="in_groff"/>
    <xsl:text> </xsl:text>
  </xsl:template>

  <!-- <esc/> element: produces \e in groff !-->
  <xsl:template match="esc">
    <xsl:text>\e</xsl:text>
    <xsl:value-of select="."/>
  </xsl:template>

  <!-- <p/> element !-->
  <xsl:template match="p">
    <xsl:text>.Pp&#xa;</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <!-- <br/> element !-->
  <xsl:template match="br">
    <xsl:apply-templates/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <!-- <i/> element !-->
  <xsl:template match="i">
    <xsl:text>&#xa;.Em "</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>"&#xa;</xsl:text>
  </xsl:template>

  <!-- <i/> element, in groff mode !-->
  <xsl:template match="i" mode="in_groff">
    <xsl:text> Em "</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>" </xsl:text>
  </xsl:template>

  <!-- <b/> element !-->
  <xsl:template match="b">
    <xsl:text>&#xa;.Sy "</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>"&#xa;</xsl:text>
  </xsl:template>

  <!-- <b/> element, in groff mode !-->
  <xsl:template match="b" mode="in_groff">
    <xsl:text>Sy "</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>" </xsl:text>
  </xsl:template>

  <!-- <code/> element !-->
  <xsl:template match="code">
    <xsl:text>&#xa;.Bd -literal&#xa;</xsl:text>
    <xsl:apply-templates mode="keep-spaces"/>
    <xsl:text>&#xa;.Ed&#xa;</xsl:text>
  </xsl:template>

  <!-- <dl/> element !-->
  <xsl:template match="dl">
    <xsl:text>&#xa;.Bl -inset -offset indent&#xa;</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#xa;.El&#xa;</xsl:text>
  </xsl:template>

  <!-- <dt/> element !-->
  <xsl:template match="dt">
    <xsl:text>&#xa;.It Em </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <!-- <dd/> element !-->
  <xsl:template match="dd">
    <xsl:apply-templates/>
  </xsl:template>

  <!-- <table/> element !-->
  <xsl:template match="table">
    <xsl:text>&#xa;.Bl -column -offset indent </xsl:text>
    <xsl:for-each select="trh">
      <xsl:apply-templates/>
    </xsl:for-each>
    <xsl:text>&#xa;</xsl:text>
    <xsl:apply-templates mode="in_groff"/>
    <xsl:text>.El&#xa;</xsl:text>
  </xsl:template>

  <!-- <trh/> element !-->
  <xsl:template match="trh" mode="in_groff">
    <xsl:text>.It </xsl:text>
    <xsl:apply-templates mode="in_groff"/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <!-- <th/> element !-->
  <xsl:template match="th">
    <xsl:text>".Sy </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>" </xsl:text>
  </xsl:template>

  <!-- <th/> element, in groff mode !-->
  <xsl:template match="th" mode="in_groff">
    <xsl:if test="position() > 1">
      <xsl:text> Ta </xsl:text>
    </xsl:if>
    <xsl:text>Sy </xsl:text>
    <xsl:apply-templates mode="in_groff"/>
  </xsl:template>

  <!-- <tr/> element, in groff mode !-->
  <xsl:template match="tr" mode="in_groff">
    <xsl:text>.It </xsl:text>
    <xsl:for-each select="td">
      <xsl:call-template name="td" mode="in_groff"/>
    </xsl:for-each>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <!-- <td/> element, in groff mode !-->
  <xsl:template name="td" mode="in_groff">
    <xsl:if test="position() > 1">
      <xsl:text> Ta </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="in_groff"/>
  </xsl:template>

  <xsl:template name="tab.for.loop">
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

  <xsl:template match="tab" mode="keep-spaces">
    <xsl:call-template name="tab.for.loop">
      <xsl:with-param name="i">1</xsl:with-param>
      <xsl:with-param name="count">
        <xsl:value-of select="2*@count"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

</xsl:stylesheet>
