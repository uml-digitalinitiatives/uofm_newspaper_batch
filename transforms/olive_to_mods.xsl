<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:php="http://php.net/xsl"
  xmlns="http://www.loc.gov/mods/v3">

  <xsl:output method='xml' version='1.0' encoding='utf-8' indent='yes'/>

  <!-- A page is a page... -->
  <xsl:template match="/">
    <mods version="3.4">
      <titleInfo>
        <xsl:apply-templates mode="title"/>
      </titleInfo>

      <typeOfResource>newspaper</typeOfResource>

      <originInfo>
        <issuance>continuing</issuance>
        <frequency authority="marcfrequency">Daily</frequency>
        <xsl:apply-templates mode="origin"/>
      </originInfo>

      <relatedItem type="host">
        <xsl:apply-templates mode="related"/>
        <part>
          <xsl:apply-templates mode="related_part"/>
        </part>
      </relatedItem>
    </mods>
  </xsl:template>

  <xsl:template name="findTitle">
    <xsl:param name="code" />
    <xsl:value-of name="publication_title" select="document('../xml/Olive_newspaper_key.xml')//newspaper[codes/code = $code]/name" />
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-PAGE/Meta" mode="title">
    <title>
    <xsl:call-template name="findTitle">
      <xsl:with-param name="code" select="@PUBLICATION"/>
    </xsl:call-template>
    <xsl:if test="string-length(@ISSUE_DATE) &gt; 0">
      <xsl:text>, </xsl:text>
      <xsl:value-of select="php:functionString('uofm_newspaper_batch_fix_date', @ISSUE_DATE, 'd/m/Y')"/>
    </xsl:if>
    <xsl:if test="@PAGE_NO">
      <xsl:text> (Page </xsl:text>
      <xsl:value-of select="@PAGE_NO"/>
      <xsl:text>)</xsl:text>
    </xsl:if>
    </title>
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-PAGE/Meta" mode="related">
    <titleInfo>
      <title>
        <xsl:call-template name="findTitle">
          <xsl:with-param name="code" select="@PUBLICATION"/>
        </xsl:call-template>
      </title>
    </titleInfo>
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-PAGE/Meta" mode="related_part">
    <xsl:if test="@PAGE_NO">
      <extent unit="pages">
        <start>
          <xsl:value-of select="@PAGE_NO"/>
        </start>
      </extent>
    </xsl:if>
    <xsl:if test="@ISSUE_DATE">
      <date encoding="iso8601">
        <xsl:value-of select="php:functionString('uofm_newspaper_batch_fix_date', @ISSUE_DATE, 'd/m/Y')"/>
      </date>
    </xsl:if>
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-PAGE/Meta" mode="origin">
    <dateIssued encoding="iso8601">
      <xsl:value-of select="php:functionString('uofm_newspaper_batch_fix_date', @ISSUE_DATE, 'd/m/Y')"/>
    </dateIssued>
  </xsl:template>

  <!-- Recurse by default. -->
  <xsl:template match='*'>
    <xsl:apply-templates/>
  </xsl:template>

  <!-- Delete text which isn't handled explicitly. -->
  <xsl:template match="text()"/>
  <xsl:template match="text()" mode="title"/>
  <xsl:template match="text()" mode="related"/>
  <xsl:template match="text()" mode="related_part"/>
  <xsl:template match="text()" mode="origin"/>
</xsl:stylesheet>
