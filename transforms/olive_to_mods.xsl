<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:php="http://php.net/xsl"
  xmlns:ia="http://www.iarchives.com/schema/2002/export"
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

  <xsl:template match="Xmd_toc" mode="title">
    <xsl:apply-templates mode="printTitle" select="."/>
  </xsl:template>

  <xsl:template match="XMD-PAGE" mode="title">
    <xsl:apply-templates mode="printTitle" select="Meta"/>
  </xsl:template>

  <xsl:template match="Xmd_toc|Meta" mode="printTitle">
    <title>
    <xsl:call-template name="findTitle">
      <xsl:with-param name="code" select="@PUBLICATION"/>
    </xsl:call-template>
    <xsl:if test="string-length(@ISSUE_DATE) &gt; 0">
      <xsl:text>, </xsl:text>
      <xsl:value-of select="php:functionString('uofm_newspaper_batch_fix_date', @ISSUE_DATE)"/>
    </xsl:if>
    <xsl:if test="@PAGE_NO">
      <xsl:text> ( Page </xsl:text>
      <xsl:value-of select="@PAGE_NO"/>
      <xsl:text>)</xsl:text>
    </xsl:if>
    </title>
  </xsl:template>

  <xsl:template match="ia:header-item[@name='volume']" mode="title">
    <partNumber>
      <xsl:text>vol </xsl:text>
      <xsl:value-of select="@value"/>
      <xsl:if test="../ia:header-item[@name='number']">
        <xsl:text> no </xsl:text>
        <xsl:value-of select="../ia:header-item[@name='number']/@value"/>

        <!-- only present in pages -->
        <xsl:if test="../ia:header-item[@name='page']">
          <xsl:text> pg </xsl:text>
          <xsl:value-of select="../ia:header-item[@name='page']/@value"/>
        </xsl:if>
      </xsl:if>
    </partNumber>
  </xsl:template>

  <!-- Only present in "articles" -->
  <xsl:template match="ia:header-item[@name='headline']" mode="title">
    <subTitle>
      <xsl:value-of select="text()"/>
    </subTitle>
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


  <xsl:template match="ia:header-item[@name='volume']" mode="related_part">
    <detail>
      <xsl:attribute name="type">
        <xsl:value-of select="@name"/>
      </xsl:attribute>
      <number>
        <xsl:value-of select="@value"/>
      </number>
    </detail>
  </xsl:template>

  <xsl:template match="ia:header-item[@name='number']" mode="related_part">
    <detail type="issue">
      <number>
        <xsl:value-of select="@value"/>
      </number>
    </detail>
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-PAGE/Meta" mode="related_part">
    <extent unit="pages">
      <start>
        <xsl:value-of select="@PAGE_NO"/>
      </start>
    </extent>
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-Page/Meta" mode="related_part">
    <date encoding="iso8601">
      <xsl:value-of select="php:functionString('uofm_newspaper_batch_fix_date', @ISSUE_DATE)"/>
    </date>
  </xsl:template>

  <xsl:template match="Xmd_toc|XMD-Page/Meta" mode="origin">
    <dateIssued encoding="iso8601">
      <xsl:value-of select="php:functionString('uofm_newspaper_batch_fix_date', @ISSUE_DATE)"/>
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
