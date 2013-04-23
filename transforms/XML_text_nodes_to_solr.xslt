<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- Match any root XML node -->
  <xsl:template match="/" name="index_text_nodes_as_a_text_field">
    <xsl:apply-templates mode="flatted_text_nodes"/>
  </xsl:template>

  <!--
    Recursively only output non-empty text nodes (followed by a single space).
  -->
  <xsl:template match="text()" mode="flatted_text_nodes">
    <xsl:variable name="text" select="normalize-space(.)"/>
    <xsl:if test="$text">
      <xsl:value-of select="$text"/>
      <xsl:text> </xsl:text>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
